"""GitHub device-flow authentication — mirrors the Rust auth module."""

import json
import os
import stat
import sys
import time
import urllib.request
import urllib.parse
import webbrowser
from pathlib import Path
from typing import Optional

from .github_client import GitHubClient

GITHUB_CLIENT_ID = "Ov23li06cv5RkdEJXrhL"
DEVICE_CODE_URL = "https://github.com/login/device/code"
ACCESS_TOKEN_URL = "https://github.com/login/oauth/access_token"


def credentials_path() -> Path:
    return Path.home() / ".czdev" / "credentials"


def load_token() -> str:
    path = credentials_path()
    if not path.exists():
        print("Not logged in. Run `czdev login` first.", file=sys.stderr)
        sys.exit(1)
    data = json.loads(path.read_text())
    return data["github_token"]


def load_credentials() -> dict:
    path = credentials_path()
    if not path.exists():
        print("Not logged in. Run `czdev login` first.", file=sys.stderr)
        sys.exit(1)
    return json.loads(path.read_text())


def save_credentials(creds: dict):
    path = credentials_path()
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(creds, indent=2))
    os.chmod(path, stat.S_IRUSR | stat.S_IWUSR)


def login():
    print("Requesting device code from GitHub...")

    data = urllib.parse.urlencode({
        "client_id": GITHUB_CLIENT_ID,
        "scope": "public_repo",
    }).encode()
    req = urllib.request.Request(DEVICE_CODE_URL, data=data, method="POST")
    req.add_header("Accept", "application/json")
    resp = urllib.request.urlopen(req)
    device_resp = json.loads(resp.read().decode())

    device_code = device_resp["device_code"]
    user_code = device_resp["user_code"]
    verification_uri = device_resp["verification_uri"]
    interval = device_resp.get("interval", 5)

    print()
    print(f"  Open:  {verification_uri}")
    print()
    print(f"  \033[1;91m{user_code}\033[0m")
    print()

    try:
        webbrowser.open(verification_uri)
    except Exception:
        pass

    print("Waiting for authorization (press Ctrl-C to cancel)...")

    token = None
    while token is None:
        time.sleep(interval)

        poll_data = urllib.parse.urlencode({
            "client_id": GITHUB_CLIENT_ID,
            "device_code": device_code,
            "grant_type": "urn:ietf:params:oauth:grant-type:device_code",
        }).encode()
        req = urllib.request.Request(ACCESS_TOKEN_URL, data=poll_data, method="POST")
        req.add_header("Accept", "application/json")
        resp = urllib.request.urlopen(req)
        token_resp = json.loads(resp.read().decode())

        if "access_token" in token_resp:
            token = token_resp["access_token"]
            break

        error = token_resp.get("error", "")
        if error == "authorization_pending":
            continue
        elif error == "slow_down":
            time.sleep(5)
            continue
        elif error == "expired_token":
            print("Device code expired, please try again.", file=sys.stderr)
            sys.exit(1)
        elif error:
            print(f"OAuth error: {error}", file=sys.stderr)
            sys.exit(1)

    gh = GitHubClient(token)
    user = gh.get_user()

    creds = {
        "github_token": token,
        "github_username": user.login,
        "created_at": str(int(time.time())),
    }
    save_credentials(creds)

    print()
    print(f"✓ Logged in as {user.login} ({user.email or ''})")
    print(f"  Token saved to {credentials_path()}")


def logout():
    path = credentials_path()
    if path.exists():
        try:
            data = json.loads(path.read_text())
            username = data.get("github_username", "")
        except Exception:
            username = ""
        path.unlink()
        if username:
            print(f"Removed credentials for {username}.")
        else:
            print("Credentials removed.")
    else:
        print("Not logged in.")
