# TBRetoch 0.6.2 native implementation status

This build repairs the unusable import workflow and adds actual basic editing and batch processing. It is **not an Evoto parity release**. The older V0.4 parity matrix describes the Electron implementation; its checkmarks do not apply to the native application.

| Native feature | Implementation | Limits |
|---|---|---|
| Add images / drag and drop | Native Unicode-safe file picker; QML URL-array bridge; per-file errors | Unsupported or damaged files are reported individually |
| JPG, PNG, TIFF, WebP | Qt content detection, EXIF orientation, deployed format plugins | Raster editing output is 8-bit |
| Camera RAW | LibRaw, camera white balance, sRGB; full decode for export | DNG fixture is tested; every camera model is not certified; no 16-bit RAW pipeline |
| HEIC / HEIF | LibHeif with packaged HEVC decoder | Upstream HEIC fixture is tested; HDR/color-management parity is absent |
| Basic color | 13 active controls with GPU preview and matching CPU export recipe | No production ICC-managed color engine |
| HSL | Hue/saturation/luminance in eight hue bands | No picker |
| Parametric tone curve | Four tone regions | No editable RGB/luma/channel spline curves |
| Crop and transform | Four crop edges, quarter-turn rotation, flips, straighten | No interactive crop handles, aspect-ratio locking, lens/perspective or AI crop |
| Detail | Edge-aware 3×3 denoise and unsharp mask | Basic spatial processing; not an AI denoiser |
| Creative effects | Vignette and deterministic grain | No glow/halation engine |
| Preview | Debounced worker preview for advanced tools; same recipes used at export | Proxy resolution; preview and full-resolution detail/grain need not match pixel for pixel |
| Presets | Atomic JSON save/load; validate before applying; undo as one edit | No catalog/group/team manager |
| Multi-image selection | Ctrl-click, select all, deselect | Selection is session-local |
| Sync | All / color / geometry / detail groups to selected images | No AI groups |
| History | Per-image undo/redo for edits, presets and sync | No persistent project database |
| Export | Full-resolution source; JPG/PNG/WebP/TIFF; atomic writes and collision-safe filenames | TIFF is 8-bit; no ICC/metadata/watermark/export-size controls |
| Export queue | Sequential worker queue, progress, cancel | Cancellation takes effect before the next image; an active image finishes |
| Language | Vietnamese default, English option | Native Qt system dialogs follow system integration |
| Team edition | No login/license/quota; image processing uses bundled local decoders | AI models are not bundled |

## Missing semantic processing

Portrait retouch, face/body geometry, semantic masks, makeup, clothing repair, subject/background replacement, sky replacement, depth-aware blur, inpainting and generative functions do not have a native engine. Their controls remain unavailable. They must not be enabled with cosmetic global filters or labeled complete.

## Validation

`ControllerTests` covers valid and invalid mixed selections, Unicode/space/#/% paths, cache fallback, JPG/PNG/TIFF/WebP, actual HEIC and synthetic DNG, full-resolution export, alpha, geometry, selective HSL, vignette, deterministic grain, preset validation, group sync, undo/redo, preview, filename collisions and queue cancellation. Twelve test cases pass locally with Qt 6.8.2.

The Windows workflow builds with Qt 6.8.2/MSVC 2022 and vcpkg, deploys Qt image plugins and decoder DLLs, constructs the offline installer, installs it into a clean directory, strips development directories from PATH, and runs installed QML import and GPU/advanced-render checks. Check the workflow conclusion for the exact source commit before distributing an installer. The render tests compare captured pixels and dimensions; a startup-only test is not a substitute.

For a Windows source build, install `libraw:x64-windows` and `libheif[core]:x64-windows`, configure CMake with the vcpkg toolchain and `BUILD_TESTING=ON`, build Release, and run CTest. HEIC regression testing requires `TBRETOCH_HEIC_FIXTURE` to point to the upstream `libheif/v1.20.2/examples/example.heic` fixture. The workflow performs these steps and verifies the deployed and installed runtime.
