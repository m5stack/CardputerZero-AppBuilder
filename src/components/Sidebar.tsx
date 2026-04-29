import type { Tab } from "../App";

const tabs: { id: Tab; label: string; icon: string }[] = [
  { id: "build", label: "Build", icon: "M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5" },
  { id: "debug", label: "Debug", icon: "M12 19V5M5 12l7-7 7 7" },
  { id: "emulator", label: "Emulator", icon: "M2 3h20v14H2zM8 21h8M12 17v4" },
  { id: "flash", label: "Flash SD", icon: "M19 21H5a2 2 0 01-2-2V5a2 2 0 012-2h11l5 5v11a2 2 0 01-2 2zM17 21v-8H7v8M7 3v5h8" },
];

interface Props {
  activeTab: Tab;
  onTabChange: (tab: Tab) => void;
}

export default function Sidebar({ activeTab, onTabChange }: Props) {
  return (
    <aside
      className="flex flex-col items-center py-2"
      style={{
        width: 72,
        minWidth: 72,
        background: "var(--m5-sidebar)",
        borderRight: "1px solid var(--m5-border)",
      }}
    >
      <div
        className="flex items-center justify-center mb-4 mt-1"
        style={{
          width: 40,
          height: 40,
          borderRadius: 8,
          background: "var(--m5-blue)",
          color: "#fff",
          fontWeight: 700,
          fontSize: 14,
        }}
      >
        M5
      </div>

      <nav className="flex-1 flex flex-col gap-1 w-full px-1.5">
        {tabs.map((t) => {
          const active = activeTab === t.id;
          return (
            <button
              key={t.id}
              onClick={() => onTabChange(t.id)}
              className="flex flex-col items-center gap-0.5 py-2 rounded-lg transition-all"
              style={{
                background: active ? "rgba(26, 115, 232, 0.15)" : "transparent",
                color: active ? "var(--m5-blue-light)" : "var(--m5-text-dim)",
              }}
            >
              <svg
                width="22"
                height="22"
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                strokeWidth="2"
                strokeLinecap="round"
                strokeLinejoin="round"
              >
                <path d={t.icon} />
              </svg>
              <span style={{ fontSize: 10, lineHeight: 1 }}>{t.label}</span>
            </button>
          );
        })}
      </nav>

      <div
        className="text-center pb-2"
        style={{ fontSize: 9, color: "var(--m5-text-dim)" }}
      >
        v0.1.0
      </div>
    </aside>
  );
}
