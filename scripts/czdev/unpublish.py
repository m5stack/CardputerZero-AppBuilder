"""Unpublish (remove) a package from the CardputerZero app store — mirrors the Rust unpublish module."""

import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

from . import auth
from .github_client import GitHubClient, Permission

TARGET_OWNER = "CardputerZero"
TARGET_REPO = "packages"


def run(package: str, version: str, arch: str = "arm64"):
    token = auth.load_token()
    gh = GitHubClient(token)
    user = gh.get_user()

    noreply = f"{user.login}@users.noreply.github.com"
    all_emails = [noreply]
    if user.email:
        all_emails.append(user.email)

    file_path = f"pool/main/{package}/{package}_{version}_{arch}.deb"

    # Verify the file exists and belongs to this user
    print(f"Checking ownership of {package} {version}...")

    # Use git sparse checkout + LFS to fetch just this one deb file
    tmp_dir = Path(tempfile.mkdtemp(prefix="czdev-unpublish-"))
    try:
        deb_local = fetch_deb_via_git(tmp_dir, file_path)
        if deb_local is None:
            print("ERROR: package not found in repository", file=sys.stderr)
            sys.exit(1)

        try:
            result = subprocess.run(
                ["dpkg-deb", "-f", str(deb_local), "Maintainer"],
                capture_output=True, text=True, check=True,
            )
            maintainer = result.stdout.strip()
        except (subprocess.CalledProcessError, FileNotFoundError):
            print("ERROR: dpkg-deb not available", file=sys.stderr)
            sys.exit(1)
    finally:
        shutil.rmtree(tmp_dir, ignore_errors=True)

    maint_email = extract_email(maintainer)
    if not any(e.lower() == maint_email.lower() for e in all_emails):
        print(f"Cannot unpublish: package maintainer '{maintainer}' does not match your account.", file=sys.stderr)
        print("  You can only remove packages you own.", file=sys.stderr)
        sys.exit(1)
    print(f"  ✓ Ownership verified ({maint_email})")

    # Determine push target
    perm = gh.check_permission(TARGET_OWNER, TARGET_REPO, user.login)
    if perm >= Permission.WRITE:
        push_owner = TARGET_OWNER
        push_repo = TARGET_REPO
        pr_head = None
    else:
        fork_name = gh.fork_repo(TARGET_OWNER, TARGET_REPO)
        parts = fork_name.split("/")
        push_owner = parts[0]
        push_repo = parts[1]
        branch = branch_name(package, version)
        pr_head = f"{user.login}:{branch}"

    print("Creating removal PR...")

    # Get base
    base_sha = gh.get_ref_sha(push_owner, push_repo, "heads/main")
    _, base_tree_sha = gh.get_commit(push_owner, push_repo, base_sha)

    # Create tree with file removed (sha: None deletes the entry)
    tree_sha = gh.create_tree(push_owner, push_repo, base_tree_sha, file_path, None)

    # Commit
    commit_msg = f"unpublish: {package} {version}"
    commit_sha = gh.create_commit(push_owner, push_repo, commit_msg, tree_sha, base_sha)

    # Branch
    branch = branch_name(package, version)
    gh.create_ref(push_owner, push_repo, branch, commit_sha)

    # PR
    head = pr_head if pr_head else branch
    pr_body = (
        f"## Remove package: `{package}` v{version}\n\n"
        f"Requested by @{user.login} (maintainer email: {maint_email}).\n\n"
        f"File: `{file_path}`\n\n"
        f"Submitted via `czdev unpublish`."
    )
    pr = gh.create_pull_request(
        TARGET_OWNER, TARGET_REPO,
        f"unpublish: {package} {version}",
        pr_body, head, "main",
    )

    print()
    print("✓ Removal PR created:")
    print(f"  {pr.html_url}")


def branch_name(package: str, version: str) -> str:
    ts = int(time.time())
    return f"unpublish/{package}-{version}-{ts}"


def extract_email(maintainer: str) -> str:
    start = maintainer.find("<")
    end = maintainer.find(">")
    if start != -1 and end != -1:
        return maintainer[start + 1:end]
    return maintainer


def fetch_deb_via_git(tmp_dir: Path, file_path: str):
    """Sparse-checkout + LFS smudge a single deb file from the packages repo."""
    remote_url = f"git@github.com:{TARGET_OWNER}/{TARGET_REPO}.git"
    cmds = [
        ["git", "init"],
        ["git", "remote", "add", "origin", remote_url],
        ["git", "lfs", "install", "--local"],
        ["git", "config", "core.sparseCheckout", "true"],
    ]
    for cmd in cmds:
        r = subprocess.run(cmd, cwd=str(tmp_dir), capture_output=True)
        if r.returncode != 0:
            return None

    # Write sparse-checkout pattern
    sparse_dir = tmp_dir / ".git" / "info"
    sparse_dir.mkdir(parents=True, exist_ok=True)
    (sparse_dir / "sparse-checkout").write_text(file_path + "\n")

    # Fetch main (depth=1) then checkout
    r = subprocess.run(
        ["git", "fetch", "--depth=1", "origin", "main"],
        cwd=str(tmp_dir), capture_output=True,
    )
    if r.returncode != 0:
        return None

    r = subprocess.run(
        ["git", "checkout", "origin/main"],
        cwd=str(tmp_dir), capture_output=True,
    )
    if r.returncode != 0:
        return None

    # LFS pull just this file
    subprocess.run(
        ["git", "lfs", "pull", "--include", file_path],
        cwd=str(tmp_dir), capture_output=True,
    )

    local_path = tmp_dir / file_path
    if local_path.exists() and local_path.stat().st_size > 200:
        return local_path
    return None
