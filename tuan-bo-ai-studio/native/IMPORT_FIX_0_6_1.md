# TBRetoch 0.6.1 — photo import repair

This is an import repair for the native branch. It is not the complete Evoto-style product required by TBRETOCH_PRODUCT_SPEC.md.

## Changes

- Use a native Qt Widgets file picker, passing Unicode paths directly to the import controller.
- Enable the center import button by disabling the pan overlay when no image is loaded.
- Add drag-and-drop with explicit QML URL-list conversion.
- Share LibRaw/LibHeif/Qt decoding between previews and full-resolution exports. RAW processing currently outputs 8-bit sRGB, not a production 16-bit RAW workflow.
- Preserve preview alpha with built-in PNG encoding and atomic cache writes.
- Fall back to a temporary cache when the profile cache cannot be written.
- Report path, decoding and cache failures per file; valid files in mixed selections still import.
- Allow Qt image allocations up to 512 MiB and reject inputs over 128 megapixels.

## Validation gates

Windows CI must pass controller tests, deployed startup, installed QML import for JPEG/PNG/WebP/TIFF/DNG/HEIC with Unicode paths, and installed rendered exposure preview. RAW regression uses a synthetic DNG and HEIC uses the upstream libheif example; this does not establish compatibility with every camera model or codec variant.

## Outstanding product work

Portrait segmentation, per-person retouch, skin/face/body/makeup/hair engines, background replacement/inpainting/depth blur, clothing cleanup, semantic batch sync, crop/perspective, RAW 16-bit/ICC and the full Evoto-style UI remain incomplete. Existing disabled controls must not be described as functioning features.
