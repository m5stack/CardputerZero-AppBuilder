//! `app-builder.json` reader. See docs/APP_BUILDER_JSON.md.

use anyhow::{Context, Result};
use serde::Deserialize;
use std::path::{Path, PathBuf};

#[derive(Debug, Deserialize, Clone)]
#[allow(dead_code)]
pub struct Manifest {
    pub package_name: String,
    #[serde(default = "default_version")]
    pub version: String,
    #[serde(default)]
    pub app_name: Option<String>,
    pub bin_name: String,
    #[serde(default)]
    pub description: String,
    #[serde(default = "default_runtime")]
    pub runtime: String,
    #[serde(default = "default_entry")]
    pub entry: String,
    #[serde(default = "default_event_entry")]
    pub event_entry: String,
    #[serde(default = "default_lvgl")]
    pub lvgl_version: String,
    #[serde(default)]
    pub caps: Vec<String>,
    #[serde(default)]
    pub assets: Vec<String>,
    /// Extra arguments passed to `cmake configure`, e.g. ["-DENABLE_FOO=ON"].
    #[serde(default)]
    pub cmake_args: Vec<String>,
    /// CMake target to build. Defaults to the bin_name if unset.
    #[serde(default)]
    pub cmake_target: Option<String>,
    /// Extra environment variables to export before launching the emulator.
    /// Values support ${APP_DIR} substitution.
    #[serde(default)]
    pub env: std::collections::HashMap<String, String>,

    /// Filled by the loader — absolute path to the project directory.
    #[serde(skip)]
    pub dir: PathBuf,
}

fn default_version() -> String { "0.1".into() }
fn default_runtime() -> String { "lvgl-dlopen".into() }
fn default_entry() -> String { "app_main".into() }
fn default_event_entry() -> String { "app_event".into() }
fn default_lvgl() -> String { "9.5".into() }

impl Manifest {
    pub fn load(dir: &Path) -> Result<Self> {
        let path = dir.join("app-builder.json");
        let raw = std::fs::read_to_string(&path)
            .with_context(|| format!("reading {}", path.display()))?;
        let mut m: Manifest = serde_json::from_str(&raw)
            .with_context(|| format!("parsing {}", path.display()))?;
        m.dir = dir.canonicalize().unwrap_or_else(|_| dir.to_path_buf());
        Ok(m)
    }

    pub fn display_name(&self) -> &str {
        self.app_name.as_deref().unwrap_or(&self.package_name)
    }
}

/// Recursively find directories that contain an `app-builder.json`.
pub fn discover(root: &Path) -> Result<Vec<Manifest>> {
    let mut out = Vec::new();
    visit(root, &mut out)?;
    Ok(out)
}

fn visit(dir: &Path, out: &mut Vec<Manifest>) -> Result<()> {
    let manifest = dir.join("app-builder.json");
    if manifest.is_file() {
        match Manifest::load(dir) {
            Ok(m) => out.push(m),
            Err(e) => eprintln!("warn: skipping {}: {e:#}", dir.display()),
        }
        // Don't descend further — one manifest per project.
        return Ok(());
    }
    if !dir.is_dir() {
        return Ok(());
    }
    let entries = match std::fs::read_dir(dir) {
        Ok(e) => e,
        Err(_) => return Ok(()),
    };
    for ent in entries.flatten() {
        let p = ent.path();
        if !p.is_dir() { continue; }
        let name = ent.file_name();
        let name = name.to_string_lossy();
        // Skip obvious noise.
        if name.starts_with('.') || name == "target" || name == "build"
            || name == "node_modules" || name == ".czdev" {
            continue;
        }
        visit(&p, out)?;
    }
    Ok(())
}
