use serde::{Deserialize, Serialize};
use std::path::PathBuf;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AppBuilderConfig {
    pub package_name: String,
    pub version: String,
    pub app_name: String,
    pub bin_name: String,
    #[serde(default)]
    pub description: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ProjectInfo {
    pub path: String,
    pub config: AppBuilderConfig,
}

#[tauri::command]
pub fn discover_projects(project_dir: String) -> Result<Vec<ProjectInfo>, String> {
    let root = PathBuf::from(&project_dir);
    if !root.exists() {
        return Err(format!("Directory does not exist: {}", project_dir));
    }
    let mut projects = Vec::new();
    scan_dir(&root, &root, &mut projects);
    Ok(projects)
}

fn scan_dir(dir: &PathBuf, root: &PathBuf, projects: &mut Vec<ProjectInfo>) {
    let config_path = dir.join("app-builder.json");
    if config_path.exists() {
        if let Ok(content) = std::fs::read_to_string(&config_path) {
            if let Ok(config) = serde_json::from_str::<AppBuilderConfig>(&content) {
                let rel_path = dir
                    .strip_prefix(root)
                    .unwrap_or(dir.as_path())
                    .to_string_lossy()
                    .to_string();
                projects.push(ProjectInfo {
                    path: if rel_path.is_empty() {
                        ".".to_string()
                    } else {
                        rel_path
                    },
                    config,
                });
            }
        }
    }
    if let Ok(entries) = std::fs::read_dir(dir) {
        for entry in entries.flatten() {
            let path = entry.path();
            if path.is_dir() {
                let name = path.file_name().unwrap_or_default().to_string_lossy();
                if !name.starts_with('.') && name != "node_modules" && name != "build" {
                    scan_dir(&path, root, projects);
                }
            }
        }
    }
}

#[tauri::command]
pub fn get_build_command(project_dir: String, project_path: String) -> Result<Vec<String>, String> {
    let full_path = PathBuf::from(&project_dir).join(&project_path);
    if !full_path.exists() {
        return Err(format!("Project path does not exist: {}", full_path.display()));
    }
    Ok(vec![
        "scons".to_string(),
        format!("-j{}", num_cpus()),
    ])
}

fn num_cpus() -> usize {
    std::thread::available_parallelism()
        .map(|n| n.get())
        .unwrap_or(4)
}

#[tauri::command]
pub fn get_build_env() -> Vec<(String, String)> {
    vec![
        ("CardputerZero".to_string(), "y".to_string()),
        ("CONFIG_REPO_AUTOMATION".to_string(), "y".to_string()),
    ]
}
