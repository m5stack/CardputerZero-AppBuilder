use anyhow::{anyhow, Context, Result};
use serde::{Deserialize, Serialize};
use std::fs;
use std::path::PathBuf;
use std::thread;
use std::time::Duration;

use crate::github::GitHubClient;

// Register your own OAuth App at https://github.com/settings/applications/new
// Device flow does not require a client secret.
const GITHUB_CLIENT_ID: &str = "Ov23li06cv5RkdEJXrhL";
const DEVICE_CODE_URL: &str = "https://github.com/login/device/code";
const ACCESS_TOKEN_URL: &str = "https://github.com/login/oauth/access_token";

#[derive(Serialize, Deserialize)]
pub struct Credentials {
    pub github_token: String,
    pub github_username: String,
    pub created_at: String,
}

pub fn credentials_path() -> Result<PathBuf> {
    let home = dirs::home_dir().ok_or_else(|| anyhow!("cannot determine home directory"))?;
    Ok(home.join(".czdev").join("credentials"))
}

pub fn load_token() -> Result<String> {
    let path = credentials_path()?;
    if !path.exists() {
        return Err(anyhow!(
            "not logged in. Run `czdev login` first."
        ));
    }
    let data = fs::read_to_string(&path).context("reading credentials")?;
    let creds: Credentials = serde_json::from_str(&data).context("parsing credentials")?;
    Ok(creds.github_token)
}

pub fn load_credentials() -> Result<Credentials> {
    let path = credentials_path()?;
    if !path.exists() {
        return Err(anyhow!("not logged in. Run `czdev login` first."));
    }
    let data = fs::read_to_string(&path).context("reading credentials")?;
    serde_json::from_str(&data).context("parsing credentials")
}

fn save_credentials(creds: &Credentials) -> Result<()> {
    let path = credentials_path()?;
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent).context("creating ~/.czdev")?;
    }
    let json = serde_json::to_string_pretty(creds)?;
    fs::write(&path, &json).context("writing credentials")?;

    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        fs::set_permissions(&path, fs::Permissions::from_mode(0o600))?;
    }
    Ok(())
}

#[derive(Deserialize)]
struct DeviceCodeResponse {
    device_code: String,
    user_code: String,
    verification_uri: String,
    interval: u64,
}

#[derive(Deserialize)]
struct TokenResponse {
    access_token: Option<String>,
    error: Option<String>,
}

pub fn login() -> Result<()> {
    let client = reqwest::blocking::Client::new();

    println!("Requesting device code from GitHub...");
    let resp: DeviceCodeResponse = client
        .post(DEVICE_CODE_URL)
        .header("Accept", "application/json")
        .form(&[
            ("client_id", GITHUB_CLIENT_ID),
            ("scope", "public_repo"),
        ])
        .send()
        .context("requesting device code")?
        .json()
        .context("parsing device code response")?;

    println!();
    println!("  Open:  {}", resp.verification_uri);
    println!("  Code:  {}", resp.user_code);
    println!();

    let _ = open::that(&resp.verification_uri);

    println!("Waiting for authorization (press Ctrl-C to cancel)...");

    let token = loop {
        thread::sleep(Duration::from_secs(resp.interval));

        let token_resp: TokenResponse = client
            .post(ACCESS_TOKEN_URL)
            .header("Accept", "application/json")
            .form(&[
                ("client_id", GITHUB_CLIENT_ID),
                ("device_code", resp.device_code.as_str()),
                ("grant_type", "urn:ietf:params:oauth:grant-type:device_code"),
            ])
            .send()
            .context("polling for token")?
            .json()
            .context("parsing token response")?;

        if let Some(token) = token_resp.access_token {
            break token;
        }

        match token_resp.error.as_deref() {
            Some("authorization_pending") => continue,
            Some("slow_down") => {
                thread::sleep(Duration::from_secs(5));
                continue;
            }
            Some("expired_token") => return Err(anyhow!("device code expired, please try again")),
            Some(e) => return Err(anyhow!("OAuth error: {e}")),
            None => continue,
        }
    };

    let gh = GitHubClient::new(&token);
    let user = gh.get_user().context("verifying token")?;

    let creds = Credentials {
        github_token: token,
        github_username: user.login.clone(),
        created_at: chrono_now(),
    };
    save_credentials(&creds)?;

    println!();
    println!("✓ Logged in as {} ({})", user.login, user.email.unwrap_or_default());
    println!("  Token saved to {:?}", credentials_path()?);
    Ok(())
}

pub fn logout() -> Result<()> {
    let path = credentials_path()?;
    if path.exists() {
        let creds = load_credentials().ok();
        fs::remove_file(&path).context("removing credentials")?;
        if let Some(c) = creds {
            println!("Removed credentials for {}.", c.github_username);
        } else {
            println!("Credentials removed.");
        }
    } else {
        println!("Not logged in.");
    }
    Ok(())
}

fn chrono_now() -> String {
    // Simple ISO-8601 without pulling in chrono crate
    use std::time::SystemTime;
    let dur = SystemTime::now()
        .duration_since(SystemTime::UNIX_EPOCH)
        .unwrap_or_default();
    format!("{}", dur.as_secs())
}
