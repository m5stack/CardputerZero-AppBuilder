import { useState } from "react";
import { Command } from "@tauri-apps/plugin-shell";
import TerminalOutput from "./TerminalOutput";

interface Props {
  projectDir: string;
}

export default function DebugPanel({ projectDir }: Props) {
  const [host, setHost] = useState("192.168.50.150");
  const [user, setUser] = useState("pi");
  const [lines, setLines] = useState<string[]>([]);
  const [connected, setConnected] = useState(false);

  async function deploy() {
    if (!projectDir) {
      setLines(["Please open a project first (Build tab)"]);
      return;
    }
    setLines(["Deploying to device..."]);

    const cmd = Command.create("scp", [
      "-r",
      `${projectDir}/dist/`,
      `${user}@${host}:/home/${user}/dist`,
    ]);

    cmd.stdout.on("data", (d) => setLines((p) => [...p, d]));
    cmd.stderr.on("data", (d) => setLines((p) => [...p, d]));
    cmd.on("close", (r) => {
      if (r.code === 0) {
        setConnected(true);
        setLines((p) => [...p, "Deploy successful!"]);
      } else {
        setLines((p) => [...p, `Deploy failed (exit code ${r.code})`]);
      }
    });

    await cmd.spawn();
  }

  async function runOnDevice() {
    setLines(["Running on device..."]);
    const cmd = Command.create("ssh", [
      `${user}@${host}`,
      "cd /home/pi/dist && ./M5CardputerZero-*",
    ]);

    cmd.stdout.on("data", (d) => setLines((p) => [...p, d]));
    cmd.stderr.on("data", (d) => setLines((p) => [...p, d]));
    cmd.on("close", (r) => {
      setLines((p) => [...p, `Process exited (code ${r.code})`]);
    });

    await cmd.spawn();
  }

  return (
    <div className="flex flex-col h-full">
      {/* Header */}
      <div
        className="flex items-center gap-4 px-5 py-3"
        style={{ borderBottom: "1px solid var(--m5-border)" }}
      >
        <h2 className="text-base font-semibold" style={{ color: "#eee" }}>
          Debug
        </h2>
        <span style={{ fontSize: 12, color: "var(--m5-text-dim)" }}>
          Deploy & run on device
        </span>
      </div>

      {/* Connection settings */}
      <div
        className="flex items-center gap-4 px-5 py-3"
        style={{ background: "var(--m5-bg-light)" }}
      >
        <label style={{ fontSize: 12, color: "var(--m5-text-dim)" }}>Host</label>
        <input
          value={host}
          onChange={(e) => setHost(e.target.value)}
          className="m5-input"
          style={{ width: 180 }}
        />
        <label style={{ fontSize: 12, color: "var(--m5-text-dim)" }}>User</label>
        <input
          value={user}
          onChange={(e) => setUser(e.target.value)}
          className="m5-input"
          style={{ width: 100 }}
        />
      </div>

      {/* Actions */}
      <div
        className="flex items-center gap-3 px-5 py-3"
        style={{ borderBottom: "1px solid var(--m5-border)" }}
      >
        <button onClick={deploy} className="m5-btn m5-btn-blue">
          Deploy
        </button>
        <button
          onClick={runOnDevice}
          disabled={!connected}
          className="m5-btn m5-btn-green"
        >
          Run on Device
        </button>
      </div>

      {/* Terminal */}
      <div className="flex-1 min-h-0 flex flex-col p-4">
        <TerminalOutput lines={lines} />
      </div>
    </div>
  );
}
