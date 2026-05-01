//! `czdev watch` — poll source mtimes; on change, rebuild and relaunch the
//! emulator. Intentionally dependency-free (no notify/inotify crate) — the
//! polling tick is coarse (500 ms) so false wakeups are cheap.

use crate::build::build_app;
use crate::manifest::Manifest;
use crate::paths;
use anyhow::{anyhow, Context, Result};
use std::collections::HashMap;
use std::path::{Path, PathBuf};
use std::process::{Child, Command};
use std::time::{Duration, SystemTime};

pub fn run(app_dir: &Path) -> Result<()> {
    let manifest = Manifest::load(app_dir)?;
    let repo_root = paths::repo_root(app_dir)?;
    let emulator_bin = paths::emulator_binary(&repo_root);
    let emulator_build = paths::emulator_build(&repo_root);

    if !emulator_bin.is_file() {
        return Err(anyhow!(
            "emulator not built. Run `czdev run {}` once first to build it.",
            app_dir.display()
        ));
    }

    // Build once, launch once.
    let mut child = rebuild_and_launch(&manifest, &emulator_bin, &emulator_build)?;
    let watch_roots = watch_roots(&manifest.dir);
    let mut snapshot = snapshot_mtimes(&watch_roots);

    println!("[watch] watching {} path(s); Ctrl-C to exit", watch_roots.len());
    loop {
        std::thread::sleep(Duration::from_millis(500));

        // Reap crashed emulator so a ghost pid doesn't linger.
        if let Ok(Some(status)) = child.try_wait() {
            println!("[watch] emulator exited ({status}); relaunching on next change");
        }

        let fresh = snapshot_mtimes(&watch_roots);
        if fresh == snapshot { continue; }
        snapshot = fresh;

        println!("[watch] change detected; rebuilding");
        let _ = child.kill();
        let _ = child.wait();
        match rebuild_and_launch(&manifest, &emulator_bin, &emulator_build) {
            Ok(c) => child = c,
            Err(e) => {
                eprintln!("[watch] rebuild failed: {e:#}");
                // Keep watching; user will save again.
                child = Command::new("true").spawn()?; // dummy child to keep typing
            }
        }
    }
}

fn rebuild_and_launch(
    manifest: &Manifest,
    emulator_bin: &Path,
    emulator_build: &Path,
) -> Result<Child> {
    let out = build_app(&manifest.dir)?;
    let staged = emulator_build.join("apps").join(out.lib_path.file_name().unwrap());
    std::fs::create_dir_all(staged.parent().unwrap())?;
    std::fs::copy(&out.lib_path, &staged)
        .with_context(|| format!("staging {} → {}", out.lib_path.display(), staged.display()))?;

    println!("=> launching emulator");
    let child = Command::new(emulator_bin)
        .current_dir(emulator_build)
        .arg(&staged)
        .spawn()
        .context("spawning emulator")?;
    Ok(child)
}

fn watch_roots(app_dir: &Path) -> Vec<PathBuf> {
    let mut out = Vec::new();
    for sub in ["src", "include", "assets"] {
        let p = app_dir.join(sub);
        if p.is_dir() { out.push(p); }
    }
    let cml = app_dir.join("CMakeLists.txt");
    if cml.is_file() { out.push(cml); }
    let abj = app_dir.join("app-builder.json");
    if abj.is_file() { out.push(abj); }
    out
}

fn snapshot_mtimes(roots: &[PathBuf]) -> HashMap<PathBuf, SystemTime> {
    let mut out = HashMap::new();
    for r in roots {
        walk(r, &mut out);
    }
    out
}

fn walk(path: &Path, out: &mut HashMap<PathBuf, SystemTime>) {
    let meta = match std::fs::metadata(path) {
        Ok(m) => m,
        Err(_) => return,
    };
    if meta.is_file() {
        if let Ok(mt) = meta.modified() {
            out.insert(path.to_path_buf(), mt);
        }
        return;
    }
    if !meta.is_dir() { return; }
    let entries = match std::fs::read_dir(path) {
        Ok(e) => e,
        Err(_) => return,
    };
    for ent in entries.flatten() {
        let name = ent.file_name();
        let s = name.to_string_lossy();
        if s.starts_with('.') || s == "build" || s == "target" || s == ".czdev" {
            continue;
        }
        walk(&ent.path(), out);
    }
}
