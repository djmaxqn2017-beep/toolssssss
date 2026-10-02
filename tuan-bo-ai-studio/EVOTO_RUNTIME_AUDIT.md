# Evoto Windows Runtime Audit — verified official builds

Audit date: 2026-10-02

This audit used official Evoto download/CDN endpoints on clean GitHub-hosted Windows Server 2025 runners. It records installer/runtime architecture and public application metadata only. Proprietary binaries/model weights/assets are not redistributed or reused by TBRetoch.

## Stable download-page build: Evoto 7.3.5-185
- Official page resolved Windows installer: `Evoto_Setup_7.3.5-185.exe`.
- Installer bytes: 2,037,597,856.
- SHA-256: `CB0AEB69E877A40BD9F8ED2305DA1A5A048608027B3E9EF692CD955BD2257433`.
- Authenticode: Valid.
- Signer: Truesight Technology Inc.
- Silent install `/S`: exit code 0.
- Install root: `C:\Program Files\Evoto`.
- Installed files: 1,814.
- Main runtime product version: 7.3.5.0.
- Major processes/binaries observed:
  - `Evoto.exe` — 297,899,920 bytes.
  - `Evoto-worker.exe` — 282,190,224 bytes.
  - `Evoto-camera-link.exe` — 26,794,896 bytes.
  - `Evoto Photo Editor.exe` — 13,361,040 bytes.
- Qt/WebEngine stack present: `Qt5WebEngineCore.dll`, `Qt5Gui.dll`, `Qt5Core.dll`, `Qt5Widgets.dll`, `Qt5Quick.dll`.
- FFmpeg codec/filter libraries and OpenGL software-fallback libraries are present.
- Large packaged resource container: `resources\pixcook` ~1.436 GB.
- App launched successfully on the clean runner.

## Current major build: Evoto 8.0.0-679
- Official CDN package verified and installed: `Evoto_Setup_8.0.0-679.exe`.
- Installer bytes: 2,200,893,184.
- SHA-256: `87D5F968C738929034D9A8C89F236FA69FD460304F45D636FD3C498DB2663792`.
- Authenticode: Valid.
- Signer: Truesight Technology Inc.
- Silent install `/S`: exit code 0.
- Install root: `C:\Program Files\Evoto`.
- Installed files: 1,857.
- Main runtime product version: 8.0.0.0.
- Installed file mix included roughly 843 PNG, 621 JSON, 143 DLL, 65 MP4, 57 PAK, 29 QM, 22 JPG, 19 MP3, 12 BIN, 12 EXE and 10 WMV files.
- Major binaries observed:
  - `Evoto.exe` — 241,151,888 bytes.
  - `Evoto-worker.exe` — 235,821,968 bytes.
  - `Evoto-camera-link.exe` — 25,340,304 bytes.
  - `Evoto Photo Editor.exe` — 15,410,064 bytes.
- Qt/WebEngine runtime remains present: `Qt5WebEngineCore.dll` ~115.7 MB plus Qt GUI/Core/Widgets/Quick libraries.
- FFmpeg (`avcodec-60.dll`, `avfilter-9.dll`), OpenGL software fallback, ICU and Direct3D compiler runtime are present.
- Large packaged resource container: `resources\pixcook` ~1.599 GB.
- Installed resources include version/demo media such as `asset_hub.wmv` and large local effect configuration/assets such as `resources\effect\config\SkinTexture\texture.png`.
- App launched successfully on the clean runner.

## Architectural conclusions relevant to TBRetoch
1. Evoto is not architected as a single browser/UI process doing per-pixel JavaScript work. The installation exposes a large native Qt/WebEngine application plus a separate `Evoto-worker.exe` processing executable.
2. A dedicated worker/process boundary is therefore a sensible architecture target for TBRetoch: UI interactions/pan/zoom stay independent from image processing and model inference.
3. Evoto ships a substantial local resource package and many local JSON/PNG/media/config resources. TBRetoch should likewise deploy its own legally redistributable offline models/resources locally rather than fetching them per edit.
4. `Evoto-camera-link.exe` confirms tether/camera ingestion is treated as a separate subsystem. TBRetoch should isolate optional tethering from the core editor.
5. Qt/OpenGL/Direct3D/FFmpeg presence confirms a native multimedia/GPU-oriented runtime rather than a preview-canvas-only design. TBRetoch V0.5 must move CPU pixel loops and AI inference off the UI thread and use GPU/native processing paths where available.
6. Export must use an original-resolution render graph separate from the interactive preview graph.

## UI observation limitation
The app was successfully launched on a headless GitHub Windows runner, but Windows UI Automation returned no control tree and the runner has no normal interactive desktop session. Feature/control parity therefore comes from Evoto's official current documentation and release notes, while this runtime audit verifies packaging, versions, process separation and runtime architecture.

## Safety / IP boundary
The official installers were deleted from the runners before audit artifacts were uploaded. TBRetoch does not copy Evoto code, private model weights, proprietary resource bundles, icons, or branded assets. Feature parity is implemented independently with TB branding and appropriately licensed/offline components.
