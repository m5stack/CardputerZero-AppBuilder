# `app-builder.json` schema

Every directory in an application repository that should be discovered, built
and packaged contains one `app-builder.json` at its root. The file plays two
roles:

1. **Packaging** — feeds the CI `.deb` pipeline (existing behaviour).
2. **Desktop dev loop** — tells `czdev` how to build and run the app inside the
   emulator (new fields, optional).

Fields added for the desktop loop are all optional; a file that only contains
the packaging fields keeps working exactly as before.

## Full schema

```jsonc
{
  // ── Packaging (existing) ─────────────────────────────────────────
  "package_name": "hello_cz",       // Debian package name (lowercase, dash)
  "version":      "0.1",            // SemVer-ish; goes into control file
  "app_name":     "Hello CZ",       // Display name in APPLaunch
  "bin_name":     "hello_cz",       // Shared-object basename (no lib prefix)
  "description":  "Desktop dev hello app",

  // ── Desktop dev (new, all optional) ──────────────────────────────
  "runtime":       "lvgl-dlopen",   // "lvgl-dlopen" (default) | "legacy-deb-only"
  "entry":         "app_main",      // C symbol name, default "app_main"
  "event_entry":   "app_event",     // optional, default "app_event"
  "lvgl_version":  "9.5",           // host must match on major.minor
  "caps": [                         // capabilities the app declares it uses
    "keyboard",
    "audio",
    "network"
  ],
  "assets": [                       // paths relative to app dir; copied next
    "assets/fonts/",                // to the built library under its app dir
    "assets/sprites/"
  ]
}
```

## Field reference

| Field | Required | Type | Default | Used by |
|---|---|---|---|---|
| `package_name` | yes | string | — | deb, czdev |
| `version` | no | string | `"0.1"` | deb |
| `app_name` | no | string | same as `package_name` | deb, czdev |
| `bin_name` | yes | string | — | deb, czdev |
| `description` | no | string | `""` | deb |
| `runtime` | no | `"lvgl-dlopen"` \| `"legacy-deb-only"` | `"lvgl-dlopen"` | czdev |
| `entry` | no | string | `"app_main"` | czdev, emulator |
| `event_entry` | no | string | `"app_event"` | czdev, emulator |
| `lvgl_version` | no | string | `"9.5"` | czdev (refuses mismatch) |
| `caps` | no | string[] | `[]` | czdev (future: sandboxing) |
| `assets` | no | string[] | `[]` | czdev (stage next to lib) |

## Runtime modes

- **`lvgl-dlopen`** — the app is a shared library that exports the `entry` and
  optional `event_entry` symbols described in `cz_app.h`. `czdev run` loads it
  into the emulator via `dlopen` / `LoadLibrary`. This is the default for new
  apps.
- **`legacy-deb-only`** — the project is a standalone executable (Framebuffer,
  SDL, Qt, Python, …). `czdev` skips it during `run`; the CI `.deb` pipeline
  still produces a package. Use this for examples in
  `CardputerZero-Examples/` that are not LVGL.

Apps that omit `runtime` are treated as `lvgl-dlopen` iff they export
`app_main`; otherwise `czdev` prints a clear error and suggests setting
`"runtime": "legacy-deb-only"`.

## Capabilities (`caps`)

Reserved vocabulary (enforcement comes later):

| Cap | Means |
|---|---|
| `keyboard` | Uses the 44-key physical keyboard / emulator skin |
| `audio` | Plays audio via ALSA on device, SDL_mixer in emulator |
| `network` | Reads hostname / IP / Wi-Fi state |
| `filesystem` | Writes into `/usr/share/APPLaunch` or the emu sandbox |
| `pty` | Spawns a PTY (terminal-style apps) |
| `process` | fork/exec of sub-processes |

Today `caps` is metadata only; a future milestone uses it to decide which
HAL shims the emulator pre-loads.
