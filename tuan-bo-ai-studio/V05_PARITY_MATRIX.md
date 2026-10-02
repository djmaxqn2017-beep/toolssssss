# TBRetoch V0.5 — Evoto parity status matrix

Legend:
- ✅ Existing functional foundation
- 🟡 Partial / needs replacement
- ❌ Missing

This matrix is intentionally strict. A UI control without correct semantic processing is marked missing or partial.

| Area | TBRetoch V0.4 status | V0.5 requirement |
|---|---|---|
| Evoto-style edit workspace | 🟡 | Rebuild exact workflow structure with TB branding; add Favorites, Search, Manual Tools, resizable gallery, reference/compare/full-screen modes |
| Pan / zoom | ✅ basic | Matrix-based fit/pan/zoom with synced compare/reference views |
| Library projects/folders/collections | ❌ | Full project/library data model |
| Gallery filters/sort/ratings/flags | ❌ | Full gallery metadata/filter system |
| Smart Culling | ❌ | Blur, blink, exposure, face cluster, story groups |
| Presets | 🟡 local single preset | Recommended/My/Team-style local preset manager + groups/import/export |
| Sync selected effect groups | ❌ | AI Color / Color / Portrait / Background / Clothing / Crop sync groups |
| Basic color | 🟡 | Production ICC-aware engine |
| Auto WB / Auto Exposure | 🟡 simplistic | Real image-analysis based corrections |
| Curves | ❌ | Parametric + RGB/Luma/R/G/B curves |
| HSL | 🟡 Hue/Sat approximation | Hue/Sat/Lum 8 bands + picker |
| Color Grading | 🟡 approximation | Proper shadows/mids/highlights wheels |
| Color Calibration | ❌ | RGB primary Hue/Saturation |
| Detail / Sharpen / Denoise | ❌ | Production sharpening + denoise |
| Grain / Glow / Post-crop vignette | 🟡 vignette only | Full creative effects incl. White Mist / Black Mist / Halation |
| Lens Correction / Transform | ❌ | Lens/profile correction and perspective transform |
| AI Color Match Quick | 🟡 RGB statistics | Semantic reference transfer with Amount/Tone/Color |
| AI Color Match Control | ❌ | Expose generated controls + semantic local masks |
| Multi-image Color Consistency | ❌ | Same-scene color matching |
| Background Color Consistency | ❌ | Background-only batch matching |
| Person Mask | 🟡 foreground mask only | Per-person semantic mask hierarchy |
| Pet Mask | ❌ | Pet detection/mask |
| Background Mask | ✅ base | Refine edge/decontamination controls |
| Custom Brush Mask | ❌ | Add/subtract/refine brush |
| Face detection / per-person edits | ❌ | Multi-face IDs + manual Add/Delete Face + per-person recipes |
| Face skin / body skin / hair / eyes / lips / teeth / clothes masks | ❌ | Portrait parser semantic masks |
| Acne / Freckles | ❌ | Dedicated detection + healing |
| Eye Bags / Dark Circles | ❌ | Dedicated semantic cleanup + lower-eyelid protect |
| Face Shine | ❌ | Specular cleanup |
| Glasses Glare | ❌ | Mild + strong glare removal |
| Nostril Cleanup | ❌ | Dedicated cleanup |
| Wrinkle zones | ❌ | Region-specific wrinkle removal |
| Double Chin / Jawline cleanup | ❌ | Semantic contour cleanup |
| Body blemish / tan line / stretch marks | ❌ | Dedicated body cleanup |
| Skin Dodge & Burn | ❌ | Texture-preserving even-skin engine |
| Sculpt D&B | ❌ | Face contour aware D&B |
| Textured Smoothing | ❌ | Texture-preserving smoothing |
| Frequency Separation | ❌ | High/Low independent controls |
| Skin Softening | 🟡 blur approximation | Replace with texture-aware engine |
| Body Complexion | ❌ | Reference/eyedropper complexion unification |
| Face Mesh | ❌ | Dense landmarks/mesh |
| Head Pose | ❌ | 3D head pose controls |
| Face symmetry | ❌ | Semantic symmetry |
| Face/Temple/Cheek/Jaw/V-shape | 🟡 bounding-box warp | Dense mesh deformation + background protection |
| Forehead/Hairline/Philtrum/Chin controls | ❌ | Full facial geometry set |
| Brow geometry | ❌ | Linked/unlinked detailed controls |
| Eye geometry | 🟡 basic size warp | Full geometry controls + L/R unlink |
| Nose geometry | 🟡 basic width warp | Full nose controls |
| Mouth geometry | ❌ | Full mouth/lip controls |
| Eye appearance | 🟡 brightness | Iris/whites/reflection/flare/red-vein/red-eye/glare/catchlights |
| Direct Gaze / Iris correction | ❌ | Per-eye iris geometry |
| Teeth cleanup/alignment | 🟡 whitening overlay | Flaw removal, edge, whitening, alignment |
| Makeup | 🟡 basic overlays | Semantic makeup styles/components |
| Hair | 🟡 shine/color approximation | Part fill, volume, hairline, stray hairs, smooth, shine, color |
| Hands | ❌ | Beautification + veins |
| Clothing & Accessories module | ❌ | First-class module incl. wrinkle cleanup |
| Pet retouch | ❌ | Pet portrait features/masks |
| Body pose / skeleton | ❌ | Pose-aware 2D/3D skeleton |
| Full Body AI Reshape | 🟡 rectangle warp | Pose-aware reshape with background protection |
| Belly Slimming | ❌ | Local body semantic reshape |
| Neck/Arms/Chest/Waist/Hips/Legs | ❌ | Detailed linked/unlinked body geometry |
| Liquify Background Repair | ❌ | Background line/texture restoration |
| Clean Backdrop | ❌ | Dedicated cleanup |
| Smart Removal / People Removal | ❌ | Inpainting/removal engine |
| Matte Refinement | ❌ | Black/white fringe cleanup |
| Background Replacement | 🟡 simple mask foundation | Full scene composite + refine + subject transform |
| AI Background Fusion | ❌ | Subject/scene harmonization |
| Character Lighting | 🟡 relight approximation | Scene-aware subject lighting |
| Floor Reflection | ❌ | Ground reflection synthesis |
| AI Sky Replacement | ❌ | Sky segmentation/replacement + color/reflection controls |
| Lens Blur | 🟡 Gaussian background blur | Depth-aware lens/bokeh engine |
| AI Set Design Batch | ❌ | Batch scene design workflow |
| Grass Fill | ❌ | Semantic ground/grass fill |
| Old Photo Restoration | ❌ | Restore + optional colorize + 2K/4K |
| Generative Expand | ❌ | Local generative canvas expansion if deployable offline model is selected |
| Healing Brush / Patch / Clone | ❌ | Full manual repair tools |
| Liquify manual tool | ❌ | Brush-based liquify + own history |
| Crop / Rotate | 🟡 rotate only | Crop, rotate, straighten, AI horizontal correction, face-position crop |
| Workflow templates | ❌ | Import → Cull → Apply Effect → Export workflow recipes |
| Export original resolution | ✅ foundation | Add full format/size/profile/sharpen/watermark/metadata controls |
| JPEG 0–100 | ✅ foundation | Quality presets + optional file-size limit |
| TIFF 8/16 bit | ❌ | Required |
| PNG | ❌/partial | Required with alpha preservation |
| ICC / metadata | 🟡 | Explicit profile + metadata controls |
| Multi-export presets | ❌ | Simultaneous variants |
| Export queue/cancel/progress | ❌ | Worker queue |
| CPU multithread | ✅ partial | Decode/encode/inference worker pools |
| GPU inference | 🟡 DirectML attempt | Must be tested on real Windows GPU before claiming complete |
| Offline after install | 🟡 | Full network-disabled acceptance test |

## Release rule
TBRetoch V0.5 will not be labeled “Evoto parity” until all priority editing rows above are ✅ and each one passes the release gate in `V05_ENGINEERING_PLAN.md`.
