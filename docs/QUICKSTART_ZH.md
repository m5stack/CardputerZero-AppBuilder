# 快速上手 — CardputerZero 桌面开发 (czdev CLI)

[English](QUICKSTART.md) | [日本語](QUICKSTART_JA.md)

无需 CardputerZero 实体设备，在你的 Mac / Linux 上 3 分钟跑起来一个 320×170 LVGL 应用。

## 1. 安装依赖

**macOS:**
```bash
brew install cmake pkg-config sdl2 sdl2_image sdl2_mixer freetype
```

**Linux (Debian/Ubuntu):**
```bash
sudo apt install -y build-essential cmake pkg-config \
    libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libfreetype-dev
```

**Windows:** 需要 MSYS2 MINGW64 环境，参考
[DESKTOP_DEV.md §4](DESKTOP_DEV.md#4-windows-lvgl--emulator--known-issues-and-plan)。
目前 macOS / Linux 流程是完整可用的。

还需要 Rust 工具链（用于编译 `czdev`）：
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

## 2. 克隆仓库（含子模块）

```bash
git clone --recursive git@github.com:m5stack/CardputerZero-AppBuilder.git
cd CardputerZero-AppBuilder
```

如果你已经克隆但忘了 `--recursive`：
```bash
git submodule update --init --recursive
```

## 3. 检查开发环境

```bash
cargo run -p czdev --release -- doctor
```

所有 required 行应该显示 OK。缺什么就按输出的提示装。

## 4. 跑 hello 示例

```bash
cargo run -p czdev --release -- run examples/hello_cz
```

第一次运行会：

1. 编译模拟器（仅一次，产物缓存在 `emulator/build/`）
2. 编译 `examples/hello_cz` 到 `.czdev/build/`
3. 把生成的 `libhello_cz.dylib`（或 `.so`）复制到模拟器 `apps/` 目录
4. 启动模拟器，通过 `dlopen` 加载你的 App

你会看到一个 320×170 的 LCD 窗口（带键盘皮肤），显示 `Hello, CardputerZero!`。
关闭窗口即退出。

## 5. 热重载开发循环

```bash
cargo run -p czdev --release -- watch examples/hello_cz
```

`watch` 会监视 `src/`、`include/`、`assets/`、`CMakeLists.txt` 和
`app-builder.json`。任何文件修改后自动重新编译并重启模拟器。

## 6. 写你自己的 App

复制 `examples/hello_cz/` 然后改 `src/hello_cz.c`。ABI 定义见
`sdk/include/cz_app.h`：

```c
#include <cz_app.h>

void app_main(lv_obj_t *parent) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, "你的界面代码");
    lv_obj_center(label);
}

void app_event(int type, void *data) {
    (void)type; (void)data;
}
```

`CMakeLists.txt` 只需三行：

```cmake
cmake_minimum_required(VERSION 3.16)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../sdk/cmake")
include(CZApp)
cz_add_lvgl_app(my_app SOURCES src/my_app.c)
```

`app-builder.json` 清单文件（详见 `docs/APP_BUILDER_JSON.md`）：

```json
{
  "package_name": "my_app",
  "bin_name": "my_app",
  "app_name": "My App",
  "runtime": "lvgl-dlopen",
  "lvgl_version": "9.5"
}
```

## 7. 部署到真机

arm64 `.deb` 通过 CI 构建——触发仓库里的 `build-deb.yml` workflow。
然后推送到设备：

```bash
cargo run -p czdev --release -- deploy \
    --host pi@192.168.50.150 \
    --deb path/to/my_app_arm64.deb
```

## czdev 命令速查

| 命令 | 作用 |
|------|------|
| `czdev doctor` | 检查依赖是否就绪（cmake, SDL2, freetype 等） |
| `czdev list [路径]` | 扫描目录下所有 `app-builder.json` 项目 |
| `czdev build [路径]` | 编译 App 的共享库（.dylib / .so） |
| `czdev run [路径]` | 编译 + 启动模拟器加载 App |
| `czdev watch [路径]` | 监视源码变化，自动重编译 + 重启 |
| `czdev deploy --host --deb` | 将 .deb 通过 SSH 推送到设备 |

所有命令通过 `cargo run -p czdev --release --` 前缀调用，或者你也可以先
`cargo install --path crates/czdev` 装到全局 PATH。

## 常见问题

- **`emulator submodule not checked out`** — `git submodule update --init --recursive`
- **LVGL 未定义符号链接错误** — 正常现象（运行时由模拟器提供），如果 linker 直接
  报 error 而不是 warning，参考 `DESKTOP_DEV.md`
- **macOS `Library not loaded: @rpath/SDL2.framework`** — `brew install sdl2`
  然后重新跑 `czdev doctor`

## 架构简图

```
┌──────────────────────────────────────────────────────────────┐
│                    cardputer-zero-emu (模拟器)                  │
│  ┌────────────────┐         ┌────────────────────────────┐   │
│  │ LVGL 9.5 引擎  │  ◄────  │  SDL2 窗口 (320×170)       │   │
│  │ + 字体/图标     │         │  + 键盘事件映射             │   │
│  └───────┬────────┘         └────────────────────────────┘   │
│          │ dlopen(RTLD_GLOBAL)                                │
│          ▼                                                    │
│  ┌────────────────┐                                          │
│  │ 你的 App .dylib │  ← app_main(parent) / app_event(...)    │
│  └────────────────┘                                          │
└──────────────────────────────────────────────────────────────┘
```
