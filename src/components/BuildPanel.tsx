import { useState } from "react";
import { invoke } from "@tauri-apps/api/core";
import { Command } from "@tauri-apps/plugin-shell";
import { open } from "@tauri-apps/plugin-dialog";
import TerminalOutput from "./TerminalOutput";
import DockerSetup from "./DockerSetup";

interface ProjectInfo {
  path: string;
  config: {
    package_name: string;
    version: string;
    app_name: string;
    bin_name: string;
    description: string;
  };
}

interface Props {
  projectDir: string;
  setProjectDir: (dir: string) => void;
}

type BuildStatus = "idle" | "building" | "success" | "error";

const statusMeta: Record<BuildStatus, { color: string; label: string }> = {
  idle: { color: "#666", label: "Ready" },
  building: { color: "#ecc94b", label: "Building..." },
  success: { color: "var(--m5-green)", label: "Success" },
  error: { color: "#f56565", label: "Failed" },
};

export default function BuildPanel({ projectDir, setProjectDir }: Props) {
  const [projects, setProjects] = useState<ProjectInfo[]>([]);
  const [selectedProject, setSelectedProject] = useState<number>(0);
  const [buildStatus, setBuildStatus] = useState<BuildStatus>("idle");
  const [lines, setLines] = useState<string[]>([]);
  const [dockerReady, setDockerReady] = useState(false);

  async function openProject() {
    const dir = await open({ directory: true, title: "Open Project" });
    if (!dir) return;
    setProjectDir(dir as string);
    setLines([]);
    setBuildStatus("idle");
    try {
      const found: ProjectInfo[] = await invoke("discover_projects", {
        projectDir: dir as string,
      });
      setProjects(found);
      setSelectedProject(0);
      setLines([`Found ${found.length} project(s):`]);
      found.forEach((p) =>
        setLines((prev) => [...prev, `  ${p.config.app_name} (${p.path})`])
      );
    } catch (e) {
      setLines([`Error: ${e}`]);
    }
  }

  async function startBuild() {
    if (!projectDir || projects.length === 0) return;
    const project = projects[selectedProject];
    setBuildStatus("building");
    setLines(["Starting native build..."]);

    const fullPath = `${projectDir}/${project.path}`;
    const cmd = Command.create("scons", [
      `-j${navigator.hardwareConcurrency || 4}`,
    ], {
      cwd: fullPath,
      env: {
        CardputerZero: "y",
        CONFIG_REPO_AUTOMATION: "y",
      },
    });

    attachBuildHandlers(cmd);
    await cmd.spawn();
  }

  async function startDockerBuild() {
    if (!projectDir || projects.length === 0) return;
    const project = projects[selectedProject];
    setBuildStatus("building");

    try {
      const rt: string = await invoke("get_runtime");
      setLines([`Starting build via ${rt}...`]);

      const args: string[] = await invoke("docker_build_project", {
        projectDir,
        projectPath: project.path,
      });

      const cmd = Command.create(rt, args);
      attachBuildHandlers(cmd);
      await cmd.spawn();
    } catch (e) {
      setBuildStatus("error");
      setLines((prev) => [...prev, `Error: ${e}`]);
    }
  }

  function attachBuildHandlers(cmd: ReturnType<typeof Command.create>) {
    cmd.stdout.on("data", (data) => setLines((prev) => [...prev, data]));
    cmd.stderr.on("data", (data) => setLines((prev) => [...prev, data]));
    cmd.on("close", (result) => {
      if (result.code === 0) {
        setBuildStatus("success");
        setLines((prev) => [...prev, "", "Build successful!"]);
      } else {
        setBuildStatus("error");
        setLines((prev) => [...prev, "", `Build failed (exit code ${result.code})`]);
      }
    });
    cmd.on("error", (err) => {
      setBuildStatus("error");
      setLines((prev) => [...prev, `Error: ${err}`]);
    });
  }

  const st = statusMeta[buildStatus];

  return (
    <div className="flex flex-col h-full">
      {/* Header bar */}
      <div
        className="flex items-center gap-4 px-5 py-3"
        style={{ borderBottom: "1px solid var(--m5-border)" }}
      >
        <h2 className="text-base font-semibold" style={{ color: "#eee" }}>
          Build
        </h2>
        <span style={{ fontSize: 12, color: "var(--m5-text-dim)" }}>
          Cross-compile to ARM64
        </span>
        <div className="flex-1" />
        <div className="flex items-center gap-2">
          <div
            className="rounded-full"
            style={{
              width: 8,
              height: 8,
              background: st.color,
              boxShadow: buildStatus === "building" ? `0 0 6px ${st.color}` : "none",
            }}
          />
          <span style={{ fontSize: 12, color: st.color }}>{st.label}</span>
        </div>
      </div>

      {/* Docker status bar */}
      <DockerSetup onStatusChange={setDockerReady} />

      {/* Toolbar */}
      <div
        className="flex items-center gap-3 px-5 py-3"
        style={{ background: "var(--m5-bg-light)" }}
      >
        <button onClick={openProject} className="m5-btn m5-btn-blue">
          Open Project
        </button>
        {projectDir && (
          <span
            className="truncate"
            style={{ fontSize: 12, color: "var(--m5-text-dim)", maxWidth: 400 }}
          >
            {projectDir}
          </span>
        )}
      </div>

      {/* Project selector + build buttons */}
      {projects.length > 0 && (
        <div
          className="flex items-center gap-3 px-5 py-3"
          style={{ borderBottom: "1px solid var(--m5-border)" }}
        >
          <select
            value={selectedProject}
            onChange={(e) => setSelectedProject(Number(e.target.value))}
            className="m5-select"
          >
            {projects.map((p, i) => (
              <option key={i} value={i}>
                {p.config.app_name} ({p.path})
              </option>
            ))}
          </select>

          <button
            onClick={startBuild}
            disabled={buildStatus === "building"}
            className="m5-btn m5-btn-green"
          >
            {buildStatus === "building" ? "Building..." : "Build"}
          </button>

          {dockerReady && (
            <button
              onClick={startDockerBuild}
              disabled={buildStatus === "building"}
              className="m5-btn m5-btn-blue"
            >
              {buildStatus === "building" ? "Building..." : "Build via Docker"}
            </button>
          )}
        </div>
      )}

      {/* Terminal */}
      <div className="flex-1 min-h-0 flex flex-col p-4">
        <TerminalOutput lines={lines} />
      </div>
    </div>
  );
}
