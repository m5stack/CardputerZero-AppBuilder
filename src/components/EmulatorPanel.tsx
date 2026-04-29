import { useState } from "react";
import { Command } from "@tauri-apps/plugin-shell";
import TerminalOutput from "./TerminalOutput";

interface Props {
  projectDir: string;
}

export default function EmulatorPanel({ projectDir }: Props) {
  const [lines, setLines] = useState<string[]>([]);
  const [running, setRunning] = useState(false);

  async function startEmulator() {
    if (!projectDir) {
      setLines(["Please open a project first (Build tab)"]);
      return;
    }

    setRunning(true);
    setLines(["Starting QEMU aarch64 emulator..."]);

    const distDir = `${projectDir}/dist`;

    const cmd = Command.create("qemu-aarch64", [
      "-L",
      "/usr/aarch64-linux-gnu",
      `${distDir}/M5CardputerZero-UserDemo`,
    ], {
      cwd: distDir,
    });

    cmd.stdout.on("data", (d) => setLines((p) => [...p, d]));
    cmd.stderr.on("data", (d) => setLines((p) => [...p, d]));
    cmd.on("close", (r) => {
      setRunning(false);
      setLines((p) => [...p, `Emulator exited (code ${r.code})`]);
    });
    cmd.on("error", (err) => {
      setRunning(false);
      setLines((p) => [
        ...p,
        `Error: ${err}`,
        "",
        "Make sure qemu-user-static is installed:",
        "  sudo apt install qemu-user-static",
        "  # or: brew install qemu",
      ]);
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
          Emulator
        </h2>
        <span style={{ fontSize: 12, color: "var(--m5-text-dim)" }}>
          QEMU user-mode ARM64 emulation
        </span>
      </div>

      {/* Actions */}
      <div
        className="flex items-center gap-3 px-5 py-3"
        style={{
          background: "var(--m5-bg-light)",
          borderBottom: "1px solid var(--m5-border)",
        }}
      >
        <button
          onClick={startEmulator}
          disabled={running}
          className="m5-btn m5-btn-blue"
        >
          {running ? "Running..." : "Start Emulator"}
        </button>
        <span style={{ fontSize: 11, color: "var(--m5-text-dim)" }}>
          SDL2 window displays the 320x170 LVGL UI
        </span>
      </div>

      {/* Terminal */}
      <div className="flex-1 min-h-0 flex flex-col p-4">
        <TerminalOutput lines={lines} />
      </div>
    </div>
  );
}
