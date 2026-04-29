import { useEffect, useRef } from "react";

interface Props {
  lines: string[];
}

export default function TerminalOutput({ lines }: Props) {
  const bottomRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    bottomRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [lines]);

  return (
    <div
      className="h-full w-full rounded-lg font-mono text-xs p-3 overflow-y-auto"
      style={{ background: "#1a1a1a", border: "1px solid var(--m5-border)" }}
    >
      {lines.length === 0 ? (
        <span style={{ color: "#555" }}>Waiting for output...</span>
      ) : (
        lines.map((line, i) => (
          <div
            key={i}
            style={{
              color:
                line.includes("error") || line.includes("Error")
                  ? "#f56565"
                  : line.includes("warning") || line.includes("Warning")
                    ? "#ecc94b"
                    : line.includes("success") || line.includes("Success")
                      ? "var(--m5-green)"
                      : "#b0b0b0",
              lineHeight: 1.6,
            }}
          >
            {line}
          </div>
        ))
      )}
      <div ref={bottomRef} />
    </div>
  );
}
