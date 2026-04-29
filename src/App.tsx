import { useState } from "react";
import Sidebar from "./components/Sidebar";
import BuildPanel from "./components/BuildPanel";
import DebugPanel from "./components/DebugPanel";
import EmulatorPanel from "./components/EmulatorPanel";
import FlashPanel from "./components/FlashPanel";

export type Tab = "build" | "debug" | "emulator" | "flash";

function App() {
  const [tab, setTab] = useState<Tab>("build");
  const [projectDir, setProjectDir] = useState<string>("");

  return (
    <div className="flex h-screen" style={{ background: "var(--m5-bg)" }}>
      <Sidebar activeTab={tab} onTabChange={setTab} />
      <main className="flex-1 flex flex-col overflow-hidden">
        <div className={`flex-1 flex flex-col overflow-hidden ${tab === "build" ? "" : "hidden"}`}>
          <BuildPanel projectDir={projectDir} setProjectDir={setProjectDir} />
        </div>
        <div className={`flex-1 flex flex-col overflow-hidden ${tab === "debug" ? "" : "hidden"}`}>
          <DebugPanel projectDir={projectDir} />
        </div>
        <div className={`flex-1 flex flex-col overflow-hidden ${tab === "emulator" ? "" : "hidden"}`}>
          <EmulatorPanel projectDir={projectDir} />
        </div>
        <div className={`flex-1 flex flex-col overflow-hidden ${tab === "flash" ? "" : "hidden"}`}>
          <FlashPanel />
        </div>
      </main>
    </div>
  );
}

export default App;
