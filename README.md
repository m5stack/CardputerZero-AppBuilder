# M5Stack AppBuilder

Online build system for [M5CardputerZero](https://docs.m5stack.com/) applications. Submit any public Git repository and get a ready-to-install `.deb` package — no local toolchain required.

## How It Works

1. Go to **Actions** > **Build DEB Package** > **Run workflow**
2. Fill in the form:

   | Field | Required | Example | Description |
   |-------|----------|---------|-------------|
   | **Repository URL** | Yes | `https://github.com/eggfly/M5CardputerZero-UserDemo.git` | Any public HTTP Git URL (GitHub, GitCode, Gitee, etc.) |
   | **Branch** | No | `master` | Leave empty to use the repository's default branch |

3. The system automatically scans for `app-builder.json` files in the repo, builds each project, and packages them as `.deb`
4. Download the `.deb` from the workflow run's **Artifacts** section

### app-builder.json

Place this file in each project directory that should be built:

```json
{
  "package_name": "userdemo",
  "version": "0.1",
  "app_name": "UserDemo",
  "bin_name": "M5CardputerZero-UserDemo",
  "description": "M5CardputerZero User Demo Application"
}
```

### Install on Device

```bash
scp <package>_arm64.deb pi@<device-ip>:/tmp/
ssh pi@<device-ip> "sudo dpkg -i /tmp/<package>_arm64.deb"
```

## Architecture

The CI pipeline runs on x86_64 and **cross-compiles** to ARM64 (aarch64) using the `aarch64-linux-gnu-` toolchain — the same approach used by the [M5Stack_Linux_Libs](https://github.com/m5stack/M5Stack_Linux_Libs) SDK. This is significantly faster than emulated ARM64 builds.

```
User Input (repo URL)
        │
        ▼
  ┌──────────────┐     ┌──────────────┐     ┌──────────────┐     ┌──────────────┐
  │  git clone   │────▶│   discover   │────▶│ scons build  │────▶│  dpkg-deb    │
  │  --recursive │     │ app-builder  │     │ (x86→arm64)  │     │  packaging   │
  └──────────────┘     │    .json     │     └──────────────┘     └──────────────┘
                       └──────────────┘              │
                              │                      ▼
                        N projects          N × .deb artifacts
                        (parallel)            (download)
```

## DEB Package Structure

Generated packages follow the [APPLaunch packaging conventions](https://github.com/dianjixz/M5CardputerZero-UserDemo/blob/main/doc/APPLaunch-App-%E6%89%93%E5%8C%85%E6%8C%87%E5%8D%97.md):

```
<package>.deb
├── DEBIAN/
│   ├── control
│   ├── postinst      (enable & start systemd service)
│   └── prerm         (stop & disable service)
├── lib/systemd/system/
│   └── <package>.service
└── usr/share/APPLaunch/
    ├── applications/<package>.desktop
    ├── bin/<executable>
    ├── lib/
    └── share/
        ├── font/*.ttf
        └── images/*.png
```

## Future: Desktop IDE

M5Stack AppBuilder is planned to become a cross-platform desktop IDE (Windows, macOS, Linux) for building M5CardputerZero applications locally — a visual scaffold for beginners and hobbyists to create, build, and deploy apps to their devices.

## Related Projects

- [M5CardputerZero-UserDemo](https://github.com/dianjixz/M5CardputerZero-UserDemo) — Reference user demo application
- [M5Stack_Linux_Libs](https://github.com/m5stack/M5Stack_Linux_Libs) — SDK with SCons build system
- [m5stack-linux-dtoverlays](https://github.com/m5stack/m5stack-linux-dtoverlays) — Device tree overlays & drivers

## License

MIT
