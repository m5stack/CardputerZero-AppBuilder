//! `czdev list` — scan a directory for app-builder.json files and print them.

use crate::manifest;
use anyhow::Result;
use std::path::Path;

pub fn run(root: &Path) -> Result<()> {
    let projects = manifest::discover(root)?;
    if projects.is_empty() {
        println!("No app-builder.json files found under {}", root.display());
        return Ok(());
    }
    println!("{:<24}  {:<16}  {:<8}  path", "name", "runtime", "lvgl");
    println!("{0:-<24}  {0:-<16}  {0:-<8}  {0:-<40}", "");
    for p in projects {
        println!(
            "{:<24}  {:<16}  {:<8}  {}",
            p.display_name(),
            p.runtime,
            p.lvgl_version,
            p.dir.display()
        );
    }
    Ok(())
}
