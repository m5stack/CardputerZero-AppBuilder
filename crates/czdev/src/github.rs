use anyhow::{anyhow, Context, Result};
use reqwest::blocking::Client;
use reqwest::header::{ACCEPT, AUTHORIZATION, USER_AGENT};
use serde::{Deserialize, Serialize};

const GITHUB_API: &str = "https://api.github.com";

pub struct GitHubClient {
    token: String,
    client: Client,
}

#[derive(Deserialize, Debug)]
pub struct User {
    pub login: String,
    pub email: Option<String>,
}

#[derive(Deserialize, Debug)]
pub struct UserEmail {
    pub email: String,
    pub verified: bool,
    pub primary: bool,
}

#[derive(Deserialize, Debug)]
struct PermissionResponse {
    permission: String,
}

#[derive(Deserialize, Debug)]
struct RefObject {
    sha: String,
}

#[derive(Deserialize, Debug)]
struct RefResponse {
    object: RefObject,
}

#[derive(Deserialize, Debug)]
struct CommitTreeRef {
    sha: String,
}

#[derive(Deserialize, Debug)]
struct CommitResponse {
    sha: String,
    tree: CommitTreeRef,
}

#[derive(Deserialize, Debug)]
struct BlobResponse {
    sha: String,
}

#[derive(Deserialize, Debug)]
struct TreeResponse {
    sha: String,
}

#[derive(Deserialize, Debug)]
struct CreateCommitResponse {
    sha: String,
}

#[derive(Deserialize, Debug)]
pub struct PullRequestResponse {
    pub html_url: String,
    pub number: u64,
}

#[derive(Serialize)]
struct BlobRequest {
    content: String,
    encoding: String,
}

#[derive(Serialize)]
struct TreeEntry {
    path: String,
    mode: String,
    #[serde(rename = "type")]
    entry_type: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    sha: Option<String>,
}

#[derive(Serialize)]
struct CreateTreeRequest {
    base_tree: String,
    tree: Vec<TreeEntry>,
}

#[derive(Serialize)]
struct CreateCommitRequest {
    message: String,
    tree: String,
    parents: Vec<String>,
}

#[derive(Serialize)]
struct CreateRefRequest {
    #[serde(rename = "ref")]
    ref_name: String,
    sha: String,
}

#[derive(Serialize)]
struct CreatePrRequest {
    title: String,
    body: String,
    head: String,
    base: String,
}

#[derive(PartialEq, PartialOrd)]
pub enum Permission {
    None,
    Read,
    Write,
    Admin,
}

impl GitHubClient {
    pub fn new(token: &str) -> Self {
        Self {
            token: token.to_string(),
            client: Client::new(),
        }
    }

    fn get(&self, path: &str) -> reqwest::blocking::RequestBuilder {
        self.client
            .get(format!("{GITHUB_API}{path}"))
            .header(AUTHORIZATION, format!("Bearer {}", self.token))
            .header(USER_AGENT, "czdev/0.1")
            .header(ACCEPT, "application/vnd.github+json")
    }

    fn post(&self, path: &str) -> reqwest::blocking::RequestBuilder {
        self.client
            .post(format!("{GITHUB_API}{path}"))
            .header(AUTHORIZATION, format!("Bearer {}", self.token))
            .header(USER_AGENT, "czdev/0.1")
            .header(ACCEPT, "application/vnd.github+json")
    }

    pub fn get_user(&self) -> Result<User> {
        self.get("/user")
            .send()
            .context("GET /user")?
            .error_for_status()
            .context("GET /user status")?
            .json()
            .context("parsing user")
    }

    pub fn get_user_emails(&self) -> Result<Vec<UserEmail>> {
        self.get("/user/emails")
            .send()
            .context("GET /user/emails")?
            .error_for_status()
            .context("GET /user/emails status")?
            .json()
            .context("parsing emails")
    }

    pub fn get_verified_emails(&self) -> Result<Vec<String>> {
        let emails = self.get_user_emails()?;
        Ok(emails
            .into_iter()
            .filter(|e| e.verified)
            .map(|e| e.email)
            .collect())
    }

    pub fn check_permission(&self, owner: &str, repo: &str, username: &str) -> Result<Permission> {
        let resp = self
            .get(&format!("/repos/{owner}/{repo}/collaborators/{username}/permission"))
            .send()
            .context("checking permission")?;

        if resp.status() == reqwest::StatusCode::NOT_FOUND {
            return Ok(Permission::None);
        }

        let pr: PermissionResponse = resp
            .error_for_status()
            .context("permission status")?
            .json()
            .context("parsing permission")?;

        Ok(match pr.permission.as_str() {
            "admin" => Permission::Admin,
            "maintain" | "write" => Permission::Write,
            "read" | "triage" => Permission::Read,
            _ => Permission::None,
        })
    }

    pub fn fork_repo(&self, owner: &str, repo: &str) -> Result<String> {
        #[derive(Deserialize)]
        struct ForkResp {
            full_name: String,
        }
        let resp: ForkResp = self
            .post(&format!("/repos/{owner}/{repo}/forks"))
            .json(&serde_json::json!({}))
            .send()
            .context("forking repo")?
            .error_for_status()
            .context("fork status")?
            .json()
            .context("parsing fork")?;
        Ok(resp.full_name)
    }

    pub fn get_ref_sha(&self, owner: &str, repo: &str, ref_name: &str) -> Result<String> {
        let resp: RefResponse = self
            .get(&format!("/repos/{owner}/{repo}/git/ref/{ref_name}"))
            .send()
            .context("GET ref")?
            .error_for_status()
            .context("GET ref status")?
            .json()
            .context("parsing ref")?;
        Ok(resp.object.sha)
    }

    pub fn get_commit(&self, owner: &str, repo: &str, sha: &str) -> Result<(String, String)> {
        let resp: CommitResponse = self
            .get(&format!("/repos/{owner}/{repo}/git/commits/{sha}"))
            .send()
            .context("GET commit")?
            .error_for_status()
            .context("GET commit status")?
            .json()
            .context("parsing commit")?;
        Ok((resp.sha, resp.tree.sha))
    }

    pub fn create_blob(&self, owner: &str, repo: &str, content_base64: &str) -> Result<String> {
        let resp: BlobResponse = self
            .post(&format!("/repos/{owner}/{repo}/git/blobs"))
            .json(&BlobRequest {
                content: content_base64.to_string(),
                encoding: "base64".to_string(),
            })
            .send()
            .context("creating blob")?
            .error_for_status()
            .context("create blob status")?
            .json()
            .context("parsing blob")?;
        Ok(resp.sha)
    }

    pub fn create_tree(
        &self,
        owner: &str,
        repo: &str,
        base_tree: &str,
        path: &str,
        blob_sha: Option<&str>,
    ) -> Result<String> {
        let entry = TreeEntry {
            path: path.to_string(),
            mode: "100644".to_string(),
            entry_type: "blob".to_string(),
            sha: blob_sha.map(|s| s.to_string()),
        };
        let resp: TreeResponse = self
            .post(&format!("/repos/{owner}/{repo}/git/trees"))
            .json(&CreateTreeRequest {
                base_tree: base_tree.to_string(),
                tree: vec![entry],
            })
            .send()
            .context("creating tree")?
            .error_for_status()
            .context("create tree status")?
            .json()
            .context("parsing tree")?;
        Ok(resp.sha)
    }

    pub fn create_commit(
        &self,
        owner: &str,
        repo: &str,
        message: &str,
        tree_sha: &str,
        parent_sha: &str,
    ) -> Result<String> {
        let resp: CreateCommitResponse = self
            .post(&format!("/repos/{owner}/{repo}/git/commits"))
            .json(&CreateCommitRequest {
                message: message.to_string(),
                tree: tree_sha.to_string(),
                parents: vec![parent_sha.to_string()],
            })
            .send()
            .context("creating commit")?
            .error_for_status()
            .context("create commit status")?
            .json()
            .context("parsing commit")?;
        Ok(resp.sha)
    }

    pub fn create_ref(&self, owner: &str, repo: &str, ref_name: &str, sha: &str) -> Result<()> {
        self.post(&format!("/repos/{owner}/{repo}/git/refs"))
            .json(&CreateRefRequest {
                ref_name: format!("refs/heads/{ref_name}"),
                sha: sha.to_string(),
            })
            .send()
            .context("creating ref")?
            .error_for_status()
            .context("create ref status")?;
        Ok(())
    }

    pub fn create_pull_request(
        &self,
        owner: &str,
        repo: &str,
        title: &str,
        body: &str,
        head: &str,
        base: &str,
    ) -> Result<PullRequestResponse> {
        self.post(&format!("/repos/{owner}/{repo}/pulls"))
            .json(&CreatePrRequest {
                title: title.to_string(),
                body: body.to_string(),
                head: head.to_string(),
                base: base.to_string(),
            })
            .send()
            .context("creating PR")?
            .error_for_status()
            .context("create PR status")?
            .json()
            .context("parsing PR")
    }

    pub fn get_file_content(&self, owner: &str, repo: &str, path: &str) -> Result<Vec<u8>> {
        let resp = self
            .get(&format!("/repos/{owner}/{repo}/contents/{path}"))
            .header(ACCEPT, "application/vnd.github.raw+json")
            .send()
            .context("GET file content")?;

        if resp.status() == reqwest::StatusCode::NOT_FOUND {
            return Err(anyhow!("file not found: {path}"));
        }

        Ok(resp
            .error_for_status()
            .context("GET file status")?
            .bytes()
            .context("reading bytes")?
            .to_vec())
    }
}
