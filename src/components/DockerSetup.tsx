import { useEffect, useState } from "react";
import { invoke } from "@tauri-apps/api/core";
import { Command } from "@tauri-apps/plugin-shell";

type Status =
  | "checking"
  | "ready"
  | "no-runtime"
  | "installing-runtime"
  | "no-image"
  | "building-image"
  | "error";

interface ContainerEnv {
  runtime: string;
  image_ready: boolean;
  runtime_available: boolean;
}

interface Props {
  onStatusChange?: (ready: boolean) => void;
}

export default function DockerSetup({ onStatusChange }: Props) {
  const [status, setStatus] = useState<Status>("checking");
  const [runtime, setRuntime] = useState("docker");
  const [installMethods, setInstallMethods] = useState<string[]>([]);
  const [message, setMessage] = useState("");

  async function detect() {
    setStatus("checking");
    setMessage("");
    try {
      const env: ContainerEnv = await invoke("detect_container_env");
      setRuntime(env.runtime);

      if (env.runtime_available && env.image_ready) {
        setStatus("ready");
        onStatusChange?.(true);
        return;
      }
      if (env.runtime_available && !env.image_ready) {
        setStatus("no-image");
        onStatusChange?.(false);
        return;
      }

      const methods: string[] = await invoke("detect_install_method");
      setInstallMethods(methods);
      setStatus("no-runtime");
      onStatusChange?.(false);
    } catch (e) {
      setStatus("error");
      setMessage(String(e));
      onStatusChange?.(false);
    }
  }

  useEffect(() => { detect(); }, []);

  async function installRuntime() {
    const method = installMethods[0];
    if (!method) return;
    setStatus("installing-runtime");
    setMessage(method === "brew" ? "Installing OrbStack via Homebrew..." : "Installing Lima...");

    try {
      const steps: string[][] = await invoke("get_install_commands", { method });

      for (const step of steps) {
        const [prog, ...args] = step;
        const cmd = Command.create(prog, args);
        let lastLine = "";
        cmd.stdout.on("data", (d) => { lastLine = d.trim(); setMessage(lastLine); });
        cmd.stderr.on("data", (d) => { lastLine = d.trim(); setMessage(lastLine); });

        const result = await new Promise<number>((resolve) => {
          cmd.on("close", (r) => resolve(r.code ?? 1));
          cmd.on("error", () => resolve(1));
          cmd.spawn();
        });

        if (result !== 0) {
          setStatus("error");
          setMessage(`Install failed: ${lastLine}`);
          onStatusChange?.(false);
          return;
        }
      }

      setMessage("Runtime installed. Checking...");
      await detect();
    } catch (e) {
      setStatus("error");
      setMessage(String(e));
      onStatusChange?.(false);
    }
  }

  async function buildImage() {
    setStatus("building-image");
    setMessage("Building container image...");

    const rt: string = await invoke("get_runtime");
    const cmd = Command.create(rt, ["build", "-t", "m5stack-appbuilder:latest", "."]);

    cmd.stdout.on("data", (d) => setMessage(d.trim()));
    cmd.stderr.on("data", (d) => setMessage(d.trim()));
    cmd.on("close", (r) => {
      if (r.code === 0) {
        setStatus("ready");
        setMessage("Image built");
        onStatusChange?.(true);
      } else {
        setStatus("error");
        setMessage(`Image build failed (exit ${r.code})`);
        onStatusChange?.(false);
      }
    });
    cmd.on("error", (e) => {
      setStatus("error");
      setMessage(String(e));
      onStatusChange?.(false);
    });

    await cmd.spawn();
  }

  const colors: Record<Status, string> = {
    checking: "var(--m5-text-dim)",
    ready: "var(--m5-green)",
    "no-runtime": "#f56565",
    "installing-runtime": "#ecc94b",
    "no-image": "#ecc94b",
    "building-image": "#ecc94b",
    error: "#f56565",
  };

  const labels: Record<Status, string> = {
    checking: "Detecting...",
    ready: `${runtime} Ready`,
    "no-runtime": "No container runtime",
    "installing-runtime": "Installing...",
    "no-image": "Image not built",
    "building-image": "Building image...",
    error: "Error",
  };

  const color = colors[status];
  const pulsing = status === "installing-runtime" || status === "building-image";

  return (
    <div
      className="flex items-center gap-3 px-5 py-2"
      style={{ borderBottom: "1px solid var(--m5-border)" }}
    >
      <div className="flex items-center gap-2">
        <div
          className="rounded-full"
          style={{
            width: 8,
            height: 8,
            background: color,
            boxShadow: pulsing ? `0 0 6px ${color}` : "none",
            animation: pulsing ? "pulse 1.5s infinite" : "none",
          }}
        />
        <span style={{ fontSize: 12, color }}>Docker: {labels[status]}</span>
      </div>

      {status === "no-runtime" && installMethods.length > 0 && (
        <button
          onClick={installRuntime}
          className="m5-btn m5-btn-blue"
          style={{ fontSize: 11, padding: "3px 10px" }}
        >
          {installMethods[0] === "docker-linux"
            ? "Install Docker"
            : installMethods[0] === "brew"
              ? "Install Lima (brew)"
              : "Install Lima (curl)"}
        </button>
      )}

      {status === "no-image" && (
        <button
          onClick={buildImage}
          className="m5-btn m5-btn-blue"
          style={{ fontSize: 11, padding: "3px 10px" }}
        >
          Build Image
        </button>
      )}

      {message && (
        <span
          className="truncate"
          style={{ fontSize: 11, color: "var(--m5-text-dim)", maxWidth: 500 }}
        >
          {message}
        </span>
      )}
    </div>
  );
}
