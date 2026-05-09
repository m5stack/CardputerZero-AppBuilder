# Arduino App Lab 逆向分析

> 分析对象: `/Applications/Arduino App Lab.app` v1.0.0
> Bundle ID: `cc.arduino.applab`
> 分析日期: 2026-04-29

## 架构总览

| 层级 | 技术 |
|------|------|
| **桌面框架** | Wails v2（Go + WebView），不是 Tauri/Electron |
| **后端语言** | Go（单一 fat binary，232MB，universal binary x86_64 + arm64） |
| **前端框架** | React（react-location 路由） |
| **UI 组件库** | Adobe React Spectrum / React Aria |
| **代码编辑器** | CodeMirror |
| **动画** | Lottie（@lottiefiles/toolkit-js） |
| **国际化** | FormatJS（@formatjs/intl） |
| **认证** | Auth0 SPA（auth0-spa-js） |
| **前后端通信** | Wails WebSocket IPC |
| **自定义 URL Scheme** | `arduino-app-lab://`（深度链接） |

## 核心 Go 模块

| 模块 | 版本 | 作用 |
|------|------|------|
| `arduino/arduino-cli` | v1.4.0 | Arduino CLI 核心（编译、上传、Board 管理、库管理） |
| `arduino/arduino-app-cli` | v0.8.1-rc | App Lab 专用 CLI（App 创建、远程开发板管理） |
| `arduino/arduino-flasher-cli` | v0.5.0-rc.8 | 固件烧录（daemon / download / flash / list） |
| `arduino/pluggable-discovery-protocol-handler` | - | 可插拔设备发现协议 |
| `arduino/go-serial-utils` | - | 串口操作（1200bps touch reset 等） |
| `wailsapp/wails` | v2 | 桌面应用框架 |
| gRPC + Protobuf | - | 内部服务间通信（arduino-cli daemon 模式） |

## 功能详解

### 1. Sketch 编译与上传

- 完整集成 `arduino-cli`：Compile、Upload、BurnBootloader
- 支持多种烧录工具：esptool、bossac、avrdude、dfu-util、openocd
- 支持 GDB 调试（cortex-debug 集成）
- Protobuf 定义的 gRPC 接口：
  - `CompileRequest` / `CompileResponse`
  - `UploadResult`
  - `BurnBootloaderRequest` / `BurnBootloaderResponse`
  - `DebugRequest` / `DebugData`

### 2. 远程开发板管理

通过 `arduino-app-cli/pkg/board/remote/` 实现：

- **ADB** 连接 — Android Debug Bridge，用于连接 Arduino 设备
- **SSH** 连接 — 远程 shell 访问
- **Local** 连接 — 本地设备
- **RemoteFS** — 远程文件系统操作

### 3. Bridge API（核心设计亮点）

App = Arduino Sketch + JavaScript/Python 混合编程：

```javascript
// 前端 JS/Python 调用硬件端
Bridge.call("set_led_state", led_state)

// 硬件端（Arduino Sketch）暴露接口
Bridge.provide("set_led_state", set_led_state);
```

实现了前端应用与 MCU 之间的双向 RPC 通信。

### 4. 多 Console Tab

| Tab | 用途 |
|-----|------|
| App launch (STARTUP) | App 启动日志 |
| Python (PYTHON) | Python 脚本执行输出 |
| Serial Monitor (SERIAL_MONITOR) | 串口通信监视，支持发送消息到板子 |

Serial Monitor 显示设备信息：USB（serial）或 网络（address）。

### 5. 固件刷写（Flasher）

- 专门的 Flasher 页面，多步骤引导
- Image version 选择 + dropdown 菜单
- Preserve data 选项（保留用户数据）
- 进度条 + 错误重试机制
- WiFi 配置（`ConnectToWiFi`）
- Flasher daemon 模式运行

### 6. 用户认证 & 云服务

- Auth0 SPA 认证流程
- 支持 secure origin 检查
- 连接 Arduino Cloud（arduino.cc）

### 7. 国际化

- FormatJS 完整 i18n 框架
- 包含 displaynames、listformat、pluralrules、relativetimeformat

## 前端 npm 依赖

| 包 | 用途 |
|----|------|
| `@codemirror/state` | 代码编辑器 |
| `@react-aria/menu` | 无障碍菜单组件 |
| `@react-aria/searchfield` | 搜索框组件 |
| `@formatjs/intl` | 国际化 |
| `@lottiefiles/toolkit-js` | Lottie 动画 |
| `@rgba-image/*` | 图像处理（copy、create-image、lanczos） |
| `@rollup/plugin-commonjs` | 构建工具 |
| `@babel/helpers` | JS 编译 |

## 与 CardputerZero AppBuilder 对比

| 特性 | Arduino App Lab | CardputerZero AppBuilder |
|------|----------------|--------------------------|
| 桌面框架 | Wails（Go + WebView） | Tauri（Rust + WebView） |
| 前端 | React + React Spectrum | 待定 |
| 前后端通信 | Wails IPC + gRPC | Tauri IPC |
| 代码编辑器 | CodeMirror | 可考虑 CodeMirror / Monaco |
| 目标硬件 | Arduino UNO R4 WiFi 等 | M5Stack Cardputer / ESP32 |
| Bridge API | JS/Python ↔ Arduino Sketch | 可设计类似 ESP32 Bridge |
| 远程连接 | ADB + SSH + Local | 可考虑 WiFi / BLE |
| 固件烧录 | arduino-flasher-cli | esptool 集成 |
| 认证 | Auth0 | 待定 |

## 值得借鉴的设计

1. **Bridge API 双向 RPC** — 前端通过 `Bridge.call()` 调用 MCU，MCU 通过 `Bridge.provide()` 暴露能力，非常优雅
2. **Pluggable Discovery** — 设备发现协议可插拔，方便扩展新的连接方式
3. **多种远程连接** — ADB / SSH / Local 三种模式，覆盖不同场景
4. **Flasher 独立模块** — 固件刷写作为独立 daemon 运行，支持进度追踪和错误恢复
5. **arduino-cli 集成** — 不重新造轮子，直接嵌入官方 CLI 作为 Go 库使用
