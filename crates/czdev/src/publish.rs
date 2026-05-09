use anyhow::{anyhow, Context, Result};
use rust_i18n::t;
use sha2::{Digest, Sha256};
use std::path::{Path, PathBuf};
use std::process::Command;
use std::time::{SystemTime, UNIX_EPOCH};

use crate::auth;
use crate::github::{GitHubClient, Permission};

const TARGET_OWNER: &str = "CardputerZero";
const TARGET_REPO: &str = "packages";

struct DebMetadata {
    package: String,
    version: String,
    architecture: String,
    maintainer: String,
    maintainer_email: String,
}

pub fn run(deb: Option<&Path>) -> Result<()> {
    // Check git and git-lfs are installed
    check_git_lfs_installed()?;

    let deb_path = resolve_deb(deb)?;
    println!("Package: {}", deb_path.display());
    println!();

    let token = auth::load_token()?;
    let gh = GitHubClient::new(&token);

    let user = gh.get_user().context("fetching user info")?;

    // Build list of emails to match against: public email + noreply
    // (no need for user:email scope)
    let noreply = format!("{}@users.noreply.github.com", user.login);
    let mut all_emails = vec![noreply];
    if let Some(ref email) = user.email {
        if !email.is_empty() {
            all_emails.push(email.clone());
        }
    }

    println!("{}", t!("publish.preflight"));

    // 1. Check .desktop file exists
    let has_desktop = check_desktop(&deb_path)?;
    if !has_desktop {
        return Err(anyhow!("{}", t!("publish.desktop_missing")));
    }
    println!("  ✓ {}", t!("publish.desktop_found"));

    // 2. Extract metadata and check email
    let meta = extract_metadata(&deb_path)?;
    if !all_emails.iter().any(|e| e.eq_ignore_ascii_case(&meta.maintainer_email)) {
        return Err(anyhow!(
            "{}\n  Maintainer: {}\n  Your emails: {:?}",
            t!("publish.email_mismatch"),
            meta.maintainer_email,
            all_emails
        ));
    }
    println!("  ✓ {}", t!("publish.email_match"));

    // 3. Package name validation
    if !is_valid_package_name(&meta.package) {
        return Err(anyhow!("{}: '{}'", t!("publish.pkg_invalid"), meta.package));
    }
    println!("  ✓ {} \"{}\"", t!("publish.pkg_valid"), meta.package);

    // 4. Show summary
    let file_size = std::fs::metadata(&deb_path)?.len();
    let size_mb = file_size as f64 / 1_048_576.0;
    println!(
        "  ✓ Version: {}, Arch: {}, Size: {:.1} MB",
        meta.version, meta.architecture, size_mb
    );
    println!();

    if file_size > 100 * 1024 * 1024 {
        return Err(anyhow!("{} ({:.1} MB)", t!("publish.too_large"), size_mb));
    }

    // 5. Check version is newer than existing
    check_version_newer(&gh, &meta)?;

    // Determine target: direct push or fork
    let perm = gh.check_permission(TARGET_OWNER, TARGET_REPO, &user.login)?;
    let (push_owner, push_repo, pr_head) = if perm >= Permission::Write {
        (TARGET_OWNER.to_string(), TARGET_REPO.to_string(), None)
    } else {
        println!("{} {TARGET_OWNER}/{TARGET_REPO}.", t!("publish.no_write_access"));
        print!("  → {}  ", t!("publish.forking"));
        let fork_name = gh.fork_repo(TARGET_OWNER, TARGET_REPO)?;
        println!("done ({fork_name})");
        let parts: Vec<&str> = fork_name.split('/').collect();
        (
            parts[0].to_string(),
            parts[1].to_string(),
            Some(format!("{}:{}", user.login, branch_name(&meta))),
        )
    };

    // Upload via git init + fetch + lfs push (no full clone needed)
    let file_bytes = std::fs::read(&deb_path).context("reading deb file")?;
    let sha256_hash = hex_sha256(&file_bytes);
    let file_path_in_repo = format!(
        "pool/main/{}/{}_{}_{}.deb",
        meta.package, meta.package, meta.version, meta.architecture
    );
    let branch = branch_name(&meta);
    let remote_url = format!("git@github.com:{push_owner}/{push_repo}.git");

    println!("{} {TARGET_OWNER}/{TARGET_REPO}...", t!("publish.uploading_to"));

    let tmp_dir = std::env::temp_dir().join(format!("czdev-publish-{}", std::process::id()));
    let _ = std::fs::remove_dir_all(&tmp_dir);
    std::fs::create_dir_all(&tmp_dir)?;

    // Init bare-minimum repo with LFS
    run_cmd_in(&tmp_dir, "git", &["init"])?;
    run_cmd_in(&tmp_dir, "git", &["remote", "add", "origin", &remote_url])?;
    run_cmd_in(&tmp_dir, "git", &["lfs", "install", "--local"])?;
    run_cmd_in(&tmp_dir, "git", &[
        "config", "lfs.https://github.com/CardputerZero/packages.git/info/lfs.locksverify", "false",
    ])?;

    // Fetch only the tip of main (minimal data, no LFS objects)
    print!("  → git fetch (minimal)... ");
    run_cmd_in(&tmp_dir, "git", &[
        "fetch", "--depth=1", "--filter=blob:none", "origin", "main",
    ])?;
    println!("{}", t!("publish.done"));

    // Create branch from fetched main
    print!("  → {} {branch}... ", t!("publish.creating_branch"));
    run_cmd_in(&tmp_dir, "git", &["checkout", "-b", &branch, "origin/main"])?;
    println!("{}", t!("publish.done"));

    // Copy deb into place
    let dest_dir = tmp_dir.join("pool/main").join(&meta.package);
    std::fs::create_dir_all(&dest_dir)?;
    std::fs::copy(&deb_path, dest_dir.join(format!(
        "{}_{}_{}.deb", meta.package, meta.version, meta.architecture
    )))?;

    // Add and commit
    print!("  → {} ", t!("publish.creating_commit"));
    run_cmd_in(&tmp_dir, "git", &["add", &file_path_in_repo])?;
    run_cmd_in(&tmp_dir, "git", &[
        "commit", "-m", &format!("publish: {} {} ({})", meta.package, meta.version, meta.architecture),
    ])?;
    println!("{}", t!("publish.done"));

    // Push branch (LFS upload happens automatically via SSH)
    print!("  → {} ({:.1} MB)... ", t!("publish.uploading_blob"), size_mb);
    run_cmd_in(&tmp_dir, "git", &["push", "origin", &branch])?;
    println!("{}", t!("publish.done"));

    // Cleanup
    let _ = std::fs::remove_dir_all(&tmp_dir);

    // Create PR via API
    let head = pr_head.unwrap_or_else(|| branch.clone());
    let pr_body = format!(
        "## Package: `{}`\n\n\
         | Field | Value |\n\
         |-------|-------|\n\
         | Version | {} |\n\
         | Architecture | {} |\n\
         | Maintainer | {} |\n\
         | Size | {:.1} MB |\n\
         | SHA-256 | `{}` |\n\
         | File | `{}` |\n\n\
         Submitted via `czdev publish`.",
        meta.package,
        meta.version,
        meta.architecture,
        meta.maintainer,
        size_mb,
        sha256_hash,
        file_path_in_repo,
    );
    print!("  → {} ", t!("publish.creating_pr"));
    let pr = gh.create_pull_request(
        TARGET_OWNER,
        TARGET_REPO,
        &format!("publish: {} {}", meta.package, meta.version),
        &pr_body,
        &head,
        "main",
    )?;
    println!("{}", t!("publish.done"));

    println!();
    println!("✓ {}", t!("publish.pr_created"));
    println!("  {}", pr.html_url);
    println!();
    println!("  {}", t!("publish.pr_hint"));
    Ok(())
}

fn resolve_deb(deb: Option<&Path>) -> Result<PathBuf> {
    if let Some(p) = deb {
        if !p.is_file() {
            return Err(anyhow!("{} {}", t!("common.file_not_found"), p.display()));
        }
        return Ok(p.to_path_buf());
    }
    // Search build/ for a .deb
    let build_dir = Path::new("build");
    if build_dir.is_dir() {
        let mut debs: Vec<_> = std::fs::read_dir(build_dir)?
            .filter_map(|e| e.ok())
            .filter(|e| e.path().extension().map(|x| x == "deb").unwrap_or(false))
            .collect();
        if debs.len() == 1 {
            return Ok(debs.remove(0).path());
        }
        if debs.len() > 1 {
            return Err(anyhow!(
                "multiple .deb files in build/. Specify one with --deb <path>"
            ));
        }
    }
    Err(anyhow!("no .deb file found. Specify with --deb <path>"))
}

fn check_desktop(deb: &Path) -> Result<bool> {
    let output = Command::new("dpkg-deb")
        .arg("-c")
        .arg(deb)
        .output()
        .context("running dpkg-deb -c (is dpkg-deb installed?)")?;
    let listing = String::from_utf8_lossy(&output.stdout);
    Ok(listing.lines().any(|l| l.ends_with(".desktop")))
}

fn extract_metadata(deb: &Path) -> Result<DebMetadata> {
    let fields = &["Package", "Version", "Architecture", "Maintainer"];
    let mut values = std::collections::HashMap::new();

    for field in fields {
        let output = Command::new("dpkg-deb")
            .args(["-f", &deb.to_string_lossy(), field])
            .output()
            .with_context(|| format!("dpkg-deb -f {field}"))?;
        let val = String::from_utf8_lossy(&output.stdout).trim().to_string();
        values.insert(*field, val);
    }

    let maintainer = values.get("Maintainer").cloned().unwrap_or_default();
    let email = extract_email(&maintainer);

    Ok(DebMetadata {
        package: values.get("Package").cloned().unwrap_or_default(),
        version: values.get("Version").cloned().unwrap_or_default(),
        architecture: values.get("Architecture").cloned().unwrap_or_default(),
        maintainer,
        maintainer_email: email,
    })
}

fn extract_email(maintainer: &str) -> String {
    if let Some(start) = maintainer.find('<') {
        if let Some(end) = maintainer.find('>') {
            return maintainer[start + 1..end].to_string();
        }
    }
    maintainer.to_string()
}

fn is_valid_package_name(name: &str) -> bool {
    if name.len() < 2 {
        return false;
    }
    let bytes = name.as_bytes();
    (bytes[0].is_ascii_lowercase() || bytes[0].is_ascii_digit())
        && bytes.iter().all(|&b| {
            b.is_ascii_lowercase() || b.is_ascii_digit() || b == b'.' || b == b'+' || b == b'-'
        })
}

fn branch_name(meta: &DebMetadata) -> String {
    let ts = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs();
    format!("publish/{}-{}-{}", meta.package, meta.version, ts)
}

fn hex_sha256(data: &[u8]) -> String {
    let mut hasher = Sha256::new();
    hasher.update(data);
    format!("{:x}", hasher.finalize())
}

fn check_version_newer(_gh: &GitHubClient, meta: &DebMetadata) -> Result<()> {
    // Try to fetch the Packages index to see if this package already exists
    let packages_url = format!(
        "https://cardputerzero.github.io/packages/dists/stable/main/binary-arm64/Packages"
    );
    let resp = reqwest::blocking::get(&packages_url);
    let content = match resp {
        Ok(r) if r.status().is_success() => r.text().unwrap_or_default(),
        _ => return Ok(()), // Can't check, skip (repo might be empty)
    };

    // Parse existing version for this package
    let mut in_our_package = false;
    let mut existing_version: Option<String> = None;
    for line in content.lines() {
        if line.starts_with("Package: ") {
            in_our_package = line.trim_start_matches("Package: ") == meta.package;
        }
        if in_our_package && line.starts_with("Version: ") {
            let ver = line.trim_start_matches("Version: ").to_string();
            // Keep the highest version found
            match &existing_version {
                Some(ev) if compare_versions(&ver, ev) == std::cmp::Ordering::Greater => {
                    existing_version = Some(ver);
                }
                None => existing_version = Some(ver),
                _ => {}
            }
        }
        if line.is_empty() {
            in_our_package = false;
        }
    }

    if let Some(existing) = existing_version {
        if compare_versions(&meta.version, &existing) != std::cmp::Ordering::Greater {
            return Err(anyhow!(
                "{} {} {} {}",
                meta.version, t!("publish.version_not_newer"), existing,
                "\n  Use `czdev bump` to check the next version."
            ));
        }
        println!("  ✓ {} {} {} {}", meta.version, t!("publish.version_newer"), existing, "");
    } else {
        println!("  ✓ {}", t!("publish.new_package"));
    }
    Ok(())
}

fn compare_versions(a: &str, b: &str) -> std::cmp::Ordering {
    let parse = |v: &str| -> Vec<u64> {
        v.split(|c: char| c == '.' || c == '-' || c == '~')
            .filter_map(|s| s.parse::<u64>().ok())
            .collect()
    };
    parse(a).cmp(&parse(b))
}

fn check_git_lfs_installed() -> Result<()> {
    // Check git
    if Command::new("git").arg("--version").output().is_err() {
        return Err(anyhow!(
            "git is not installed.\n  Install: https://git-scm.com/downloads"
        ));
    }
    // Check git-lfs
    let lfs_check = Command::new("git").args(["lfs", "version"]).output();
    match lfs_check {
        Ok(output) if output.status.success() => Ok(()),
        _ => Err(anyhow!(
            "git-lfs is required for publishing.\n\n  Install:\n    macOS:  brew install git-lfs\n    Linux:  sudo apt install git-lfs\n    Windows: https://git-lfs.com\n\n  Then run: git lfs install"
        )),
    }
}

fn run_cmd_in(dir: &Path, cmd: &str, args: &[&str]) -> Result<()> {
    let output = Command::new(cmd)
        .args(args)
        .current_dir(dir)
        .output()
        .with_context(|| format!("running {cmd} {}", args.join(" ")))?;
    if !output.status.success() {
        let stderr = String::from_utf8_lossy(&output.stderr);
        return Err(anyhow!("{cmd} {} failed:\n{}", args.join(" "), stderr));
    }
    Ok(())
}
