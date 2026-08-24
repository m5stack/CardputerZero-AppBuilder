# `app-builder.json` schema

Every directory in an application repository that should be discovered, built
and packaged contains one `app-builder.json` at its root. The file plays two
roles:

1. **Packaging** — feeds the CI `.deb` pipeline.
2. **AppStore listing** — the optional `store` section supplies the metadata
   that `czdev publish` (and the web portal at
   [dev.cardputer.cc](https://dev.cardputer.cc)) put on the store page: title,
   summary, screenshots, categories, icon, …

> The older desktop-emulator fields (`runtime`, `entry`, `event_entry`,
> `lvgl_version`, `caps`, `assets`) are still accepted for back-compat but are
> no longer consumed — the Rust `czdev` emulator / `czdev run` loop has been
> removed. New apps only need the packaging fields plus a `store` section.

## Full schema

```jsonc
{
  // ── Packaging (existing) ─────────────────────────────────────────
  "package_name": "hello_cz",       // Debian package name (lowercase, dash)
  "version":      "0.1",            // SemVer-ish; goes into control file
  "app_name":     "Hello CZ",       // Display name in APPLaunch
  "bin_name":     "hello_cz",       // executable / shared-object basename
  "description":  "Hello app",

  // ── AppStore listing (used by `czdev publish` + web portal) ──────
  "store": {
    "summary":     "One-line summary",        // required; ≤80 chars
    "description": "Longer detail-page text", // recommended
    "categories":  ["Games"],                 // required; 1–2 from the fixed enum
    "screenshots": ["screenshots/main.png"],  // required; 1–6 × 320×170 PNG
    "icon":        "packaging/icon.png",      // required; square PNG 128–512 (256 recommended)
    "license":     "MIT",                     // required; SPDX id
    "source_repo": "https://github.com/you/my_app", // recommended
    "author":      { "github": "you", "display_name": "Your Name" }, // display_name required
    "share_code":  "MYAP",                    // required; 4 alphanumerics, unique store-wide
    "permissions": {                          // required; exactly these 7 booleans
      "camera": false, "microphone": false, "imu": false, "network": false,
      "additional_hardware": false, "background_service": false,
      "external_display": false
    },
    "locales":     {}                         // recommended; localized title/summary
  },

  // ── Legacy desktop-emulator fields (optional, no longer consumed) ─
  "runtime":       "lvgl-dlopen",   // back-compat only
  "entry":         "app_main",
  "event_entry":   "app_event",
  "lvgl_version":  "9.5",
  "caps":          [],
  "assets":        []
}
```

## Packaging fields

| Field | Required | Type | Default | Notes |
|---|---|---|---|---|
| `package_name` | yes | string | — | Debian package name (lowercase, dash) |
| `version` | no | string | `"0.1"` | goes into the control file |
| `app_name` | no | string | same as `package_name` | display name; also the store title |
| `bin_name` | yes | string | — | executable / shared-object basename |
| `description` | no | string | `""` | control-file description |

## Store listing — `store`

Read by `czdev publish` and the web portal to build the AppStore page. The
store enforces this policy on every submission (czdev pre-checks it locally
with the same script CI runs, so a bad manifest fails before anything is
uploaded):

| Field | Required | Type | Notes |
|---|---|---|---|
| `store.summary` | **yes** | string | one-line summary shown in lists; ≤80 chars |
| `store.description` | recommended | string | detail-page text |
| `store.categories` | **yes** | string[] | 1–2 from the fixed enum below |
| `store.screenshots` | **yes** | string[] | 1–6 paths, relative to the app dir; **320×170** PNG(s) |
| `store.icon` | **yes** | string | square PNG path, 128–512 px (256 recommended) |
| `store.license` | **yes** | string | SPDX id, e.g. `MIT` |
| `store.source_repo` | recommended | string | public source URL (http/https) |
| `store.author` | **yes** | object | `{ "github": "you", "display_name": "Your Name" }` — `display_name` is what the store shows |
| `store.share_code` | **yes** | string | 4 letters/digits, unique across the store; users type it to jump to your app |
| `store.permissions` | **yes** | object | exactly 7 booleans: `camera`, `microphone`, `imu`, `network`, `additional_hardware`, `background_service`, `external_display` |
| `store.locales` | recommended | object | localized `title` / `summary` per locale (`zh-CN`, `ja`, …) |

Categories enum: `System Tools`, `Development`, `Hardware & IoT`, `Security`,
`Radio & Comms`, `AI`, `Creative & Office`, `Media`, `Games`, `Emulators`,
`Education`, `Lifestyle`, `Other`.

The store title comes from the top-level `app_name` and must be unique across
the store (case- and whitespace-insensitive).

When updating a published app, fields you leave out keep their published
values (`czdev publish` merges into the existing `meta.json` — the pinned
`uuid` and an already-assigned `share_code` survive automatically).

**First release of a new app** is not auto-merged: after the PR is created,
post a short video of the app running on a real device in the PR comments;
a maintainer merges it after watching. Version updates merge automatically
once validation passes.

## Legacy desktop-emulator fields

`runtime`, `entry`, `event_entry`, `lvgl_version`, `caps`, `assets` were used by
the old Rust `czdev` emulator (`czdev run` / `watch`). That loop has been
removed, so these fields are **no longer consumed** — they are still accepted
(and passed through by CI discovery) for back-compat, but you can safely omit
them from new manifests.
