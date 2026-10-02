# TBRetoch V0.5 — Engineering plan

This plan exists to prevent another UI-only build. Every milestone must ship working processing + tests, not placeholder controls.

## P0 — application foundation
1. Keep the Evoto-style workspace structure but TB purple branding / TB logo / TBRetoch name.
2. UI thread only handles interaction. No per-pixel loops or AI inference on UI thread.
3. Create ImageDocument model:
   - immutable original path + metadata
   - preview pyramid / proxy cache
   - nondestructive recipe
   - semantic-cache keys
   - per-person identity slots
4. Create Worker Pool:
   - decode/encode workers
   - AI inference workers
   - export workers
5. Create render scheduler:
   - interactive preview while dragging
   - low/medium proxy during motion
   - full-quality settle after slider release
6. Pan/zoom/fit uses a view transform only; it never regenerates pixels.
7. Keyboard: Space before/after, Ctrl+Z/Y, Ctrl+C/V, Shift+R Reference, Shift+Y Compare, Shift+L Loupe/Full preview.

Acceptance: 24MP JPEG preview must remain interactive while panning and dragging Basic sliders on a normal Windows editing PC.

## P1 — production color engine
- ICC-aware decode and render.
- Basic: WB, Exposure, Contrast, Highlights, Shadows, Whites, Blacks, Vibrance, Saturation.
- RGB/Luma curves.
- HSL Hue/Sat/Lum for 8 bands.
- Color Grading shadows/mids/highlights.
- Calibration RGB primaries.
- Detail: sharpening + denoise.
- Grain, vignette, glow, lens corrections.
- Auto Exposure / Auto WB.
- Full-resolution export from original source, never preview canvas.

Acceptance: preview and full-resolution export visually match within tolerance; output dimensions equal selected export settings.

## P2 — semantic foundation
Models/services must be isolated behind interfaces so models can be upgraded without rewriting UI.

Required semantic outputs:
- face boxes + dense facial landmarks/mesh
- eye / iris landmarks
- body pose / skeleton
- portrait parsing: face skin, neck, body skin, hair, brows, eyes, lips, teeth, clothes
- foreground alpha / matting
- background / sky / floor regions
- pet mask optional

Cache per image; do not recompute unchanged masks on every slider move.

Acceptance: masks are generated once, cached, and reusable by Color Match / Portrait / Background modules.

## P3 — AI Color Match
### Quick Mode
- reference image analysis
- global tone/color match
- semantic compensation for skin / hair / clothes / background
- Amount / Tone / Color controls

### Control Mode
- converts match into transparent editable controls
- generates local semantic masks
- linked/unlinked local Amount/Tone/Color

### Batch
- copy/paste recipe
- every target image runs its own semantic analysis
- same-scene Multi-Image Color Consistency
- Background Color Consistency

Acceptance: batch never copies pixel coordinates from the reference image.

## P4 — Portrait cleanup
Implement as real semantic operations, module by module:
- acne/freckle
- eye bags / dark circles / lower-eyelid protection
- face shine
- glasses glare
- nostril cleanup
- lip wrinkles/flakes
- wrinkle zones
- double chin / jawline cleanup
- body blemish / tan lines / stretch marks
- hand/body veins

Manual tuning mask required for effects where automatic result can need correction.

## P5 — skin
- Dodge & Burn even skin
- Sculpt D&B
- Textured Smoothing
- Frequency Separation high/low controls
- Skin Softening
- Body complexion / skin color unification

Rule: preserve pore/edge texture at normal values; no blanket Gaussian blur.

## P6 — face geometry
Dense mesh deformation with edge/background protection:
- head pose
- symmetry
- face / temple / cheekbone / jaw / V shape / jaw length / face width
- forehead / hairline / philtrum / middle/lower face / chin
- brows
- eyes
- nose
- mouth
- left/right linked and independent controls

Acceptance: background straight lines remain stable around face outline.

## P7 — eyes / teeth / makeup / hair
Eyes:
- brightness, eye whites, iris, reflection, flare
- red veins, red eye
- glare removal
- catchlights
- direct gaze / iris correction

Teeth:
- flaws, whitening, edge, alignment

Makeup:
- highlight, contour, brows, eyeshadow, eyeliner, eyelashes, contacts, blush, lipstick

Hair:
- part line fill, volume, hairline, stray hair cleanup, smooth, shine, color

## P8 — body geometry
Pose-aware 3D/body-mesh processing:
- AI reshape
- physique smoothing
- head size
- body / belly
- height
- torso length
- neck width/length
- arms
- chest
- waist / hips
- thigh / calf / leg length
- group-photo safety
- background repair pass after liquify

## P9 — background / scene
- clean backdrop / distraction removal
- matting refinement / edge decontamination
- background replacement
- AI background fusion
- subject transform
- subject lighting / blend
- floor reflection
- sky replacement
- lens blur / depth
- AI set design batch
- grass fill

## P10 — manual tools
- Crop / rotate / straighten
- AI horizontal correction
- Healing Brush
- Patch
- Clone Stamp
- Liquify
- history per manual tool
- Smart Remove / people removal
- Generative Expand optional online-model-free local implementation only if suitable local model is available

## P11 — library / culling / workflow
- ratings / flags / color labels
- filters and sort
- metadata
- virtual copies
- comparison/reference/full-screen/slideshow
- smart blur / blink / exposure / face cluster analysis
- story groups
- workflow recipes: Import → Cull → Apply Effect → Export

## Export requirements
- JPEG 8-bit quality 0–100
- TIFF 8/16-bit
- PNG
- original dimensions / percent / W×H / long edge / short edge
- PPI/PPC metadata
- ICC profile handling
- output sharpening presets
- watermark
- metadata choices
- queue, cancel, progress
- multi-export presets
- CPU multi-threaded encode and optional GPU-assisted processing when supported

## Runtime strategy
- Windows x64 first.
- GPU detection at startup.
- Prefer DirectML for broad AMD/Intel/NVIDIA support where model compatibility allows.
- NVIDIA path may use CUDA/TensorRT when deployed runtime and model license allow it.
- CPU fallback is mandatory.
- Display actual active backend in Diagnostics, not a fake GPU label.

## Release gate
Do not publish a version merely because installer builds. Before Release:
1. Syntax/unit tests pass.
2. CPU inference smoke test passes.
3. GPU inference smoke test passes on a real Windows GPU runner/machine before calling GPU support complete.
4. 24MP export retains source resolution.
5. Pan/zoom does not trigger image processing.
6. Basic slider drag remains interactive.
7. Offline test passes with network disabled.
8. Feature matrix marks only verified working controls as complete.
