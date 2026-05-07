# クイックスタート — CardputerZero デスクトップ開発 (czdev CLI)

[English](QUICKSTART.md) | [中文](QUICKSTART_ZH.md)

CardputerZero 実機がなくても、Mac / Linux 上で約3分で 320×170 LVGL アプリを動かせます。

## 1. 依存関係のインストール

**macOS:**
```bash
brew install cmake pkg-config sdl2 sdl2_image sdl2_mixer freetype
```

**Linux (Debian/Ubuntu):**
```bash
sudo apt install -y build-essential cmake pkg-config \
    libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libfreetype-dev
```

**Windows:** MSYS2 MINGW64 環境が必要です。詳細は
[DESKTOP_DEV.md §4](DESKTOP_DEV.md#4-windows-lvgl--emulator--known-issues-and-plan)
を参照。現状 macOS / Linux のワークフローが完全にサポートされています。

Rust ツールチェーン（`czdev` のビルドに必要）：
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

## 2. リポジトリのクローン（サブモジュール含む）

```bash
git clone --recursive git@github.com:m5stack/CardputerZero-AppBuilder.git
cd CardputerZero-AppBuilder
```

`--recursive` を付け忘れた場合：
```bash
git submodule update --init --recursive
```

## 3. 開発環境の確認

```bash
cargo run -p czdev --release -- doctor
```

required の行がすべて OK なら準備完了。MISSING と表示されたら、
出力に書かれたインストールコマンドを実行してください。

## 4. hello サンプルを実行

```bash
cargo run -p czdev --release -- run examples/hello_cz
```

初回実行時は：

1. エミュレータをビルド（初回のみ、`emulator/build/` にキャッシュ）
2. `examples/hello_cz` を `.czdev/build/` にビルド
3. 生成された `libhello_cz.dylib`（または `.so`）をエミュレータの `apps/` にコピー
4. エミュレータを起動し、`dlopen` でアプリをロード

320×170 の LCD ウィンドウ（キーボードスキン付き）に `Hello, CardputerZero!` と表示されます。
ウィンドウを閉じれば終了。

## 5. ホットリロード開発ループ

```bash
cargo run -p czdev --release -- watch examples/hello_cz
```

`watch` は `src/`、`include/`、`assets/`、`CMakeLists.txt`、
`app-builder.json` を監視し、変更を検出すると自動で再ビルド＋エミュレータ再起動します。

## 6. 自分のアプリを作る

`examples/hello_cz/` をコピーして `src/hello_cz.c` を編集。ABI 定義は
`sdk/include/cz_app.h` にあります：

```c
#include <cz_app.h>

void app_main(lv_obj_t *parent) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, "あなたのUIコード");
    lv_obj_center(label);
}

void app_event(int type, void *data) {
    (void)type; (void)data;
}
```

`CMakeLists.txt` はたった3行：

```cmake
cmake_minimum_required(VERSION 3.16)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../sdk/cmake")
include(CZApp)
cz_add_lvgl_app(my_app SOURCES src/my_app.c)
```

`app-builder.json` マニフェスト（詳細は `docs/APP_BUILDER_JSON.md`）：

```json
{
  "package_name": "my_app",
  "bin_name": "my_app",
  "app_name": "My App",
  "runtime": "lvgl-dlopen",
  "lvgl_version": "9.5"
}
```

## 7. 実機へのデプロイ

arm64 `.deb` は CI でビルドします — リポジトリの `build-deb.yml` ワークフローを
トリガーしてください。その後デバイスへ転送：

```bash
cargo run -p czdev --release -- deploy \
    --host pi@192.168.50.150 \
    --deb path/to/my_app_arm64.deb
```

## czdev コマンド一覧

| コマンド | 機能 |
|----------|------|
| `czdev doctor` | 依存関係のチェック（cmake, SDL2, freetype 等） |
| `czdev list [パス]` | ディレクトリ内の `app-builder.json` プロジェクトを一覧表示 |
| `czdev build [パス]` | アプリの共有ライブラリ（.dylib / .so）をビルド |
| `czdev run [パス]` | ビルド＋エミュレータ起動でアプリをロード |
| `czdev watch [パス]` | ソース変更を監視し、自動再ビルド＋再起動 |
| `czdev deploy --host --deb` | .deb を SSH でデバイスに転送＋インストール |

すべてのコマンドは `cargo run -p czdev --release --` のプレフィックス付きで実行します。
または `cargo install --path crates/czdev` でグローバルにインストールすることも可能です。

## トラブルシューティング

- **`emulator submodule not checked out`** — `git submodule update --init --recursive`
- **LVGL 未定義シンボルのリンクエラー** — 正常です（ランタイムにエミュレータが提供）。
  linker が error（warning ではなく）を出す場合は `DESKTOP_DEV.md` を参照
- **macOS `Library not loaded: @rpath/SDL2.framework`** — `brew install sdl2`
  してから `czdev doctor` を再実行

## アーキテクチャ概要

```
┌──────────────────────────────────────────────────────────────┐
│                cardputer-zero-emu（エミュレータ）                │
│  ┌────────────────┐         ┌────────────────────────────┐   │
│  │ LVGL 9.5 エンジン│  ◄────  │  SDL2 ウィンドウ (320×170)  │   │
│  │ + フォント/アイコン│         │  + キーボードイベントマッピング│   │
│  └───────┬────────┘         └────────────────────────────┘   │
│          │ dlopen(RTLD_GLOBAL)                                │
│          ▼                                                    │
│  ┌────────────────┐                                          │
│  │ あなたの App    │  ← app_main(parent) / app_event(...)     │
│  │ .dylib / .so   │                                          │
│  └────────────────┘                                          │
└──────────────────────────────────────────────────────────────┘
```
