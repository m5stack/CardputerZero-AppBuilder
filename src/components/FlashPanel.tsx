import { useState, useEffect } from "react";
import { Command } from "@tauri-apps/plugin-shell";
import { open } from "@tauri-apps/plugin-dialog";
import TerminalOutput from "./TerminalOutput";

interface DiskInfo {
  device: string;
  size: string;
  name: string;
}

export default function FlashPanel() {
  const [imagePath, setImagePath] = useState("");
  const [disks, setDisks] = useState<DiskInfo[]>([]);
  const [selectedDisk, setSelectedDisk] = useState("");
  const [lines, setLines] = useState<string[]>([]);
  const [flashing, setFlashing] = useState(false);

  async function detectDisks() {
    setLines(["Scanning for removable disks..."]);
    try {
      const cmd = Command.create("bash", [
        "-c",
        `diskutil list external physical 2>/dev/null || lsblk -d -o NAME,SIZE,MODEL --json 2>/dev/null`,
      ]);
      const output = await cmd.execute();
      const text = output.stdout || output.stderr || "";
      setLines(["Detected disks:", text]);

      const parsed: DiskInfo[] = [];
      const diskRegex = /\/dev\/disk(\d+)\s+\(external,\s+physical\):/g;
      let match;
      while ((match = diskRegex.exec(text)) !== null) {
        const dev = `/dev/disk${match[1]}`;
        const sizeMatch = text
          .substring(match.index)
          .match(/\*(\d+\.?\d*\s*\w+)/);
        parsed.push({
          device: dev,
          size: sizeMatch ? sizeMatch[1] : "unknown",
          name: `Disk ${match[1]}`,
        });
      }

      if (parsed.length === 0) {
        const lsblkMatch = text.match(/\{[\s\S]*\}/);
        if (lsblkMatch) {
          try {
            const json = JSON.parse(lsblkMatch[0]);
            for (const dev of json.blockdevices || []) {
              parsed.push({
                device: `/dev/${dev.name}`,
                size: dev.size || "unknown",
                name: dev.model || dev.name,
              });
            }
          } catch { /* ignore */ }
        }
      }

      setDisks(parsed);
      if (parsed.length > 0) {
        setSelectedDisk(parsed[0].device);
      } else {
        setLines((p) => [
          ...p,
          "",
          "No external disks found. Insert a TF/SD card and try again.",
        ]);
      }
    } catch (e) {
      setLines([`Error: ${e}`]);
    }
  }

  useEffect(() => {
    detectDisks();
  }, []);

  async function selectImage() {
    const path = await open({
      title: "Select OS Image",
      filters: [
        { name: "Disk Image", extensions: ["img", "iso", "gz", "xz", "zip"] },
      ],
    });
    if (path) setImagePath(path as string);
  }

  async function startFlash() {
    if (!imagePath || !selectedDisk) return;

    const rdisk = selectedDisk.replace("/dev/disk", "/dev/rdisk");

    setFlashing(true);
    setLines([
      `Flashing ${imagePath}`,
      `Target: ${selectedDisk} (${rdisk})`,
      "",
      "Unmounting disk...",
    ]);

    const unmount = Command.create("bash", [
      "-c",
      `diskutil unmountDisk ${selectedDisk} 2>/dev/null; echo done`,
    ]);
    await unmount.execute();

    setLines((p) => [...p, "Writing image (this may take several minutes)..."]);

    let ddCmd: string;
    if (imagePath.endsWith(".gz")) {
      ddCmd = `gunzip -c "${imagePath}" | sudo dd of=${rdisk} bs=4m`;
    } else if (imagePath.endsWith(".xz")) {
      ddCmd = `xz -dc "${imagePath}" | sudo dd of=${rdisk} bs=4m`;
    } else {
      ddCmd = `sudo dd if="${imagePath}" of=${rdisk} bs=4m`;
    }

    const cmd = Command.create("bash", ["-c", ddCmd]);
    cmd.stdout.on("data", (d) => setLines((p) => [...p, d]));
    cmd.stderr.on("data", (d) => setLines((p) => [...p, d]));
    cmd.on("close", (r) => {
      setFlashing(false);
      if (r.code === 0) {
        setLines((p) => [...p, "", "Flash complete! You can safely eject the card."]);
      } else {
        setLines((p) => [...p, "", `Flash failed (exit code ${r.code})`]);
      }
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
          Flash SD/TF Card
        </h2>
        <span style={{ fontSize: 12, color: "var(--m5-text-dim)" }}>
          Write OS image to removable storage
        </span>
      </div>

      {/* Image selection */}
      <div
        className="flex items-center gap-3 px-5 py-3"
        style={{ background: "var(--m5-bg-light)" }}
      >
        <button onClick={selectImage} className="m5-btn m5-btn-blue">
          Select Image
        </button>
        {imagePath ? (
          <span
            className="truncate"
            style={{ fontSize: 12, color: "var(--m5-text-dim)", maxWidth: 500 }}
          >
            {imagePath}
          </span>
        ) : (
          <span style={{ fontSize: 12, color: "#555" }}>
            .img / .gz / .xz supported
          </span>
        )}
      </div>

      {/* Disk selection + flash */}
      <div
        className="flex items-center gap-3 px-5 py-3"
        style={{ borderBottom: "1px solid var(--m5-border)" }}
      >
        <select
          value={selectedDisk}
          onChange={(e) => setSelectedDisk(e.target.value)}
          className="m5-select"
        >
          {disks.length === 0 ? (
            <option>No disks found</option>
          ) : (
            disks.map((d) => (
              <option key={d.device} value={d.device}>
                {d.device} - {d.size} ({d.name})
              </option>
            ))
          )}
        </select>
        <button
          onClick={detectDisks}
          className="m5-btn"
          style={{ background: "#444", color: "#ccc" }}
        >
          Refresh
        </button>
        <button
          onClick={startFlash}
          disabled={flashing || !imagePath || disks.length === 0}
          className="m5-btn"
          style={{
            background:
              flashing || !imagePath || disks.length === 0 ? "#444" : "#e53e3e",
            color: "#fff",
            cursor:
              flashing || !imagePath || disks.length === 0
                ? "not-allowed"
                : "pointer",
          }}
        >
          {flashing ? "Flashing..." : "Flash"}
        </button>
      </div>

      {/* Terminal */}
      <div className="flex-1 min-h-0 flex flex-col p-4">
        <TerminalOutput lines={lines} />
      </div>
    </div>
  );
}
