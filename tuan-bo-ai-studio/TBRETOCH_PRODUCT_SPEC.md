# TBRetoch Product Spec — Locked Baseline

Status: LOCKED
Product: TBRetoch
Target: Windows 10/11 x64
Edition: Internal Team Edition

## Non-negotiable product rules

- Offline-first editing and AI inference after local model pack installation.
- CPU + GPU cooperate; NVIDIA CUDA/TensorRT preferred, DirectML fallback, CPU fallback.
- Native desktop architecture (C++ / Qt / QML), not the previous Electron prototype.
- Preview is optimized and cached; final export always re-renders from the original full-resolution image.
- No license, credits, quota, login, or machine activation for the internal team edition.
- UI workflow should be familiar to experienced Evoto users while TBRetoch uses its own code, models, branding, icons and assets.
- Every feature is considered complete only when UI + real engine + preview + export + undo/redo + batch behavior (where relevant) work.
- No fake sliders.

## Branding

- Product name: TBRetoch
- Official app icon/logo source: the user-provided blue rounded-square TB artwork from the 2026-10-03 project conversation.
- Visual theme: graphite/dark workspace with blue-purple/cyan accent based on the TB logo.

## Languages

TBRetoch ships with two complete UI languages:

1. Tiếng Việt (default)
2. English

Language can be changed in-app without reinstalling. Every menu, button, slider, tooltip, dialog, error, preset, setting, installer string, model-manager message and export message must exist in both languages.

## Major modules

1. Library / Jobs / Culling
2. GPU Viewer / Before-After / Compare / Filmstrip
3. Color / Tone Curve / HSL / Color Grading
4. AI Color Match / Color Consistency
5. Portrait semantic analysis by person
6. Blemish Removal
7. Skin Retouch / Frequency Separation / Dodge & Burn
8. Facial Reshape / Face Mesh
9. Eyes / Iris / Catchlight / Direct Gaze
10. Teeth / Mouth
11. AI Makeup
12. Hair
13. Full Body Reshape / Pose / Skeleton
14. Clothing Retouch
15. Masks / semantic masks / brush / gradients
16. Background / Backdrop / Sky / Lens Blur
17. Smart Remove / Inpainting / People Removal
18. Relighting / Subject Light / Rim Light
19. Crop / Rotate / Perspective / Geometry
20. Sharpen / Denoise / AI Enhance / Upscale
21. Semantic Batch Sync
22. Export Queue / JPG / PNG / TIFF / metadata / ICC / sharpening
23. Presets
24. Cache / Performance / GPU settings / crash recovery
25. RAW + 16-bit pipeline milestone

## Architecture target

TBRetoch.exe
- UI thread, library, viewer, GPU preview, user interaction

TBRetochWorker.exe
- face/skin/hair/body/clothing/background AI inference workers

TBRetochExport.exe
- full-resolution rendering and encode queue

Shared services
- image cache
- semantic mask cache
- face landmark/mesh cache
- pose cache
- ICC/color management
- preset/project recipe serialization

## Performance targets

- Pan/zoom target: 60 FPS on supported GPUs.
- Basic slider response target: <50 ms preview response.
- Cached AI preview target: <300 ms where technically reasonable.
- Heavy inference/export may run asynchronously and must not freeze the UI.
- Preview rendering must never iterate full-resolution image pixels on the UI thread.

## Vietnamese portrait/wedding benchmark requirement

Validation set must include Vietnamese and Southeast/East Asian portraits, brides/grooms, ao dai, white wedding dresses, black hair, dyed hair, glasses, light/medium/tan skin, studio light, outdoor light, yellow wedding-hall light and LED-stage light.

Default portrait processing must not force skin whitening.

## Current engineering baseline

The old Electron V0.3/V0.4 line is deprecated. Development proceeds only from the native TBRetoch core branch.
