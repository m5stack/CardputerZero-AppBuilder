use anyhow::{anyhow, Context, Result};
use std::process::Command;
use std::time::{SystemTime, UNIX_EPOCH};

use crate::auth;
use crate::github::{GitHubClient, Permission};

const TARGET_OWNER: &str = "CardputerZero";
const TARGET_REPO: &str = "packages";

pub fn run(package: &str, version: &str, arch: &str) -> Result<()> {
    let token = auth::load_token()?;
    let gh = GitHubClient::new(&token);
    let user = gh.get_user()?;
    let verified_emails = gh.get_verified_emails()?;

    let noreply = format!("{}@users.noreply.github.com", user.login);
    let mut all_emails = verified_emails;
    if !all_emails.contains(&noreply) {
        all_emails.push(noreply);
    }

    let file_path = format!("pool/main/{}/{}_{}_{}. deb", package, package, version, arch);
    let file_path = file_path.replace(". deb", ".deb");

    // Verify the file exists and belongs to this user
    println!("Checking ownership of {package} {version}...");
    let deb_bytes = gh
        .get_file_content(TARGET_OWNER, TARGET_REPO, &file_path)
        .context("package not found in repository")?;

    // Write to temp file to inspect maintainer
    let tmp = std::env::temp_dir().join(format!("{package}_{version}_{arch}.deb"));
    std::fs::write(&tmp, &deb_bytes).context("writing temp deb")?;

    let output = Command::new("dpkg-deb")
        .args(["-f", &tmp.to_string_lossy(), "Maintainer"])
        .output()
        .context("running dpkg-deb")?;
    let maintainer = String::from_utf8_lossy(&output.stdout).trim().to_string();
    let _ = std::fs::remove_file(&tmp);

    let maint_email = extract_email(&maintainer);
    if !all_emails.iter().any(|e| e.eq_ignore_ascii_case(&maint_email)) {
        return Err(anyhow!(
            "Cannot unpublish: package maintainer '{}' does not match your account.\n  \
             You can only remove packages you own.",
            maintainer
        ));
    }
    println!("  ✓ Ownership verified ({})", maint_email);

    // Determine push target
    let perm = gh.check_permission(TARGET_OWNER, TARGET_REPO, &user.login)?;
    let (push_owner, push_repo, pr_head) = if perm >= Permission::Write {
        (TARGET_OWNER.to_string(), TARGET_REPO.to_string(), None)
    } else {
        let fork_name = gh.fork_repo(TARGET_OWNER, TARGET_REPO)?;
        let parts: Vec<&str> = fork_name.split('/').collect();
        let branch = branch_name(package, version);
        (
            parts[0].to_string(),
            parts[1].to_string(),
            Some(format!("{}:{}", user.login, branch)),
        )
    };

    println!("Creating removal PR...");

    // Get base
    let base_sha = gh.get_ref_sha(&push_owner, &push_repo, "heads/main")?;
    let (_, base_tree_sha) = gh.get_commit(&push_owner, &push_repo, &base_sha)?;

    // Create tree with file removed (sha: null deletes the entry)
    let tree_sha = gh.create_tree(&push_owner, &push_repo, &base_tree_sha, &file_path, None)?;

    // Commit
    let commit_msg = format!("unpublish: {} {}", package, version);
    let commit_sha = gh.create_commit(&push_owner, &push_repo, &commit_msg, &tree_sha, &base_sha)?;

    // Branch
    let branch = branch_name(package, version);
    gh.create_ref(&push_owner, &push_repo, &branch, &commit_sha)?;

    // PR
    let head = pr_head.unwrap_or_else(|| branch.clone());
    let pr_body = format!(
        "## Remove package: `{package}` v{version}\n\n\
         Requested by @{} (maintainer email: {}).\n\n\
         File: `{}`\n\n\
         Submitted via `czdev unpublish`.",
        user.login, maint_email, file_path
    );
    let pr = gh.create_pull_request(
        TARGET_OWNER,
        TARGET_REPO,
        &format!("unpublish: {} {}", package, version),
        &pr_body,
        &head,
        "main",
    )?;

    println!();
    println!("✓ Removal PR created:");
    println!("  {}", pr.html_url);
    Ok(())
}

fn branch_name(package: &str, version: &str) -> String {
    let ts = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs();
    format!("unpublish/{}-{}-{}", package, version, ts)
}

fn extract_email(maintainer: &str) -> String {
    if let Some(start) = maintainer.find('<') {
        if let Some(end) = maintainer.find('>') {
            return maintainer[start + 1..end].to_string();
        }
    }
    maintainer.to_string()
}
