mod build;
mod deploy;
mod doctor;
mod list;
mod manifest;
mod paths;
mod run;
mod watch;

use anyhow::Result;
use clap::{Parser, Subcommand};
use std::path::PathBuf;

/// CardputerZero desktop dev CLI.
///
/// Build, run and deploy LVGL apps for M5 CardputerZero from macOS / Linux /
/// Windows without the physical device. See docs/DESKTOP_DEV.md.
#[derive(Parser, Debug)]
#[command(name = "czdev", version, about, long_about = None)]
struct Cli {
    #[command(subcommand)]
    command: Command,
}

#[derive(Subcommand, Debug)]
enum Command {
    /// Check desktop build dependencies (SDL2, cmake, toolchain).
    Doctor,

    /// List LVGL apps discovered under a directory.
    List {
        #[arg(default_value = ".")]
        path: PathBuf,
    },

    /// Build an app's shared library for the host platform.
    Build {
        #[arg(default_value = ".")]
        path: PathBuf,
    },

    /// Build the app, launch the emulator, and load it.
    Run {
        #[arg(default_value = ".")]
        path: PathBuf,
    },

    /// Rebuild + restart the emulator whenever app sources change.
    Watch {
        #[arg(default_value = ".")]
        path: PathBuf,
    },

    /// Deploy a pre-built .deb to the device over SSH.
    Deploy {
        #[arg(default_value = ".")]
        path: PathBuf,
        /// Target host, e.g. pi@192.168.50.150
        #[arg(long)]
        host: Option<String>,
        /// Path to the .deb file (built by CI or locally).
        #[arg(long)]
        deb: Option<PathBuf>,
    },
}

fn main() -> Result<()> {
    let cli = Cli::parse();
    match cli.command {
        Command::Doctor => doctor::run(),
        Command::List { path } => list::run(&path),
        Command::Build { path } => {
            build::build_app(&path)?;
            Ok(())
        }
        Command::Run { path } => run::run_app(&path),
        Command::Watch { path } => watch::run(&path),
        Command::Deploy { path, host, deb } => deploy::run(&path, host.as_deref(), deb.as_deref()),
    }
}
