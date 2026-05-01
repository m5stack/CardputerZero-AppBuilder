//! `czdev deploy` — scp + dpkg -i a pre-built .deb to the device.
//!
//! Intentionally minimal: this does NOT cross-compile. Build the .deb via the
//! existing `build-deb.yml` CI workflow (or an on-device native build) and
//! pass the resulting path with --deb.

use anyhow::{anyhow, Context, Result};
use std::path::{Path, PathBuf};
use std::process::Command;

pub fn run(_app_dir: &Path, host: Option<&str>, deb: Option<&Path>) -> Result<()> {
    let host = host.ok_or_else(|| anyhow!("--host is required (e.g. --host pi@192.168.50.150)"))?;
    let deb = deb.ok_or_else(|| anyhow!("--deb PATH is required; build the .deb via CI or on-device first"))?;
    if !deb.is_file() {
        return Err(anyhow!(".deb file not found: {}", deb.display()));
    }

    let remote_tmp = PathBuf::from("/tmp").join(deb.file_name().unwrap());
    let remote_tmp_str = remote_tmp.to_string_lossy();

    println!("=> scp {} {}:{}", deb.display(), host, remote_tmp_str);
    let status = Command::new("scp")
        .arg(deb)
        .arg(format!("{host}:{remote_tmp_str}"))
        .status()
        .context("running scp")?;
    if !status.success() {
        return Err(anyhow!("scp failed with {status}"));
    }

    println!("=> ssh {} sudo dpkg -i {}", host, remote_tmp_str);
    let status = Command::new("ssh")
        .arg(host)
        .arg(format!("sudo dpkg -i {remote_tmp_str}"))
        .status()
        .context("running ssh dpkg -i")?;
    if !status.success() {
        return Err(anyhow!("dpkg -i failed with {status}"));
    }

    println!("deploy: ok");
    Ok(())
}
