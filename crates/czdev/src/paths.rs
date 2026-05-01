//! Locating the AppBuilder repo root and its sibling directories.

use anyhow::{anyhow, Result};
use std::path::{Path, PathBuf};

/// Walk upward from `start` until we find the AppBuilder repo root — the dir
/// that contains `sdk/cmake/CZApp.cmake`.
pub fn repo_root(start: &Path) -> Result<PathBuf> {
    let start = start.canonicalize().unwrap_or_else(|_| start.to_path_buf());
    let mut cur: &Path = &start;
    loop {
        if cur.join("sdk/cmake/CZApp.cmake").is_file() {
            return Ok(cur.to_path_buf());
        }
        match cur.parent() {
            Some(p) => cur = p,
            None => break,
        }
    }
    Err(anyhow!(
        "could not locate AppBuilder repo root (no sdk/cmake/CZApp.cmake found \
         at or above {})",
        start.display()
    ))
}

pub fn emulator_dir(repo_root: &Path) -> PathBuf { repo_root.join("emulator") }
pub fn emulator_build(repo_root: &Path) -> PathBuf { emulator_dir(repo_root).join("build") }

pub fn emulator_binary(repo_root: &Path) -> PathBuf {
    let bin = if cfg!(target_os = "windows") { "cardputer-emu.exe" } else { "cardputer-emu" };
    emulator_build(repo_root).join(bin)
}

pub fn app_lib_name(bin: &str) -> String {
    if cfg!(target_os = "macos") {
        format!("lib{bin}.dylib")
    } else if cfg!(target_os = "windows") {
        format!("lib{bin}.dll")
    } else {
        format!("lib{bin}.so")
    }
}
