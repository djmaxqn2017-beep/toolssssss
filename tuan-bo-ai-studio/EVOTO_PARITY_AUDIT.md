# TBRetoch V0.5 — Evoto Desktop parity audit

Audit date: 2026-10-03

## Scope and evidence rule

This document is the implementation baseline for TBRetoch. It is based on Evoto's public first-party Download page, Release Notes, Features catalog, Support documentation, and direct static inspection of the user-provided official Windows bootstrap installer. It does **not** copy Evoto source code, proprietary model weights, private assets, or internal configuration.

A feature is only listed as an Evoto-parity target when it is documented by Evoto publicly. If a behavior is not documented, it is marked as unknown rather than guessed.

## Direct installer audit — user-provided official Windows bootstrapper

File inspected: `EvotoInstaller_Setup_1.0.0-408_stable.exe`

Observed facts from the actual PE executable:
- Windows x86-64 GUI bootstrap installer.
- File size: 11,862,016 bytes.
- SHA-256: `c05f8bf7abd5a6176c6bc889f0eba3e4ae70a1dff376caeef2ea39327e22f951`.
- The installer is a downloader/bootstrapper rather than the full Evoto application package.
- Build-path strings identify a downloader project (`PixDownloader`) and download components such as `DownloadThread.cpp`, `Downloader.cpp`, `DownloaderManager.cpp`, `ConfigManager.cpp`, and `InstallManager.cpp`.
- It requests software/package metadata from Evoto services, including `https://api.evoto.ai/v1/app/get_software?` and country/package configuration endpoints.
- It contains resume/retry/range-download logic and verifies the downloaded package before launching installation.
- Process names referenced by the installer include `Evoto.exe`, `Evoto-worker.exe`, and `Evoto-camera-link.exe`.
- The bootstrapper refuses installation while Evoto processes are running.

Conclusion: the supplied 11.8 MB installer does **not** contain the full desktop application or its complete local model/resources payload. Static inspection is useful for installer architecture, but feature/model parity must be derived from the installed application plus official documentation. TBRetoch will therefore not pretend that this bootstrapper alone reveals Evoto's complete internal model stack.

## Current official baseline

- Latest stable Windows version on Evoto's official Download page: **7.3.0-512**.
- Download page update date: **2026-08-06**.
- Supported Windows versions listed by Evoto: **Windows 7 / 10 / 11**.
- Official Release Notes currently list **V7.3.0** as the newest desktop release, dated **2026-07-10**.
- V7.3.0 headline additions: Batch AI Set Design, Matte Refinement, Strong Glare Removal, Glow Effect, Post-Crop Vignetting.
- The public Features catalog currently exposes **97 named feature pages** across: Portrait 45, Background 18, Color 12, Clothing 5, Editing Toolkit 16, Pet Retouching 1.

TBRetoch V0.5 therefore targets the **documented Evoto 7.3 feature surface**, not an invented future version.

---

# 1. Product shell and editing workspace

Evoto documents six editing-workspace regions:

1. Top toolbar.
2. Floating widget on the left.
3. Central picture preview.
4. Bottom gallery / filmstrip.
5. Right control panel.
6. Feature modules / favorites behavior.

### Top toolbar
- Return to project/library.
- Add images.
- Undo / Redo.
- Manual tools.
- Feature search.
- Export and export-history access where appropriate.

### Floating widget
- Presets.
- Masking.
- History.

### Picture preview
- Live edited preview.
- Spacebar Before/After.
- Pan and zoom.
- Canvas background choices: default, dark, white, dark gray, medium gray, light gray.
- Preview size is independent from export resolution.
- Preferences allow configurable preview size, up to 4000 px according to Evoto support documentation.
- Optional full-size real-time color preview.
- Synced preview position and zoom setting.

### Bottom gallery
- Resizable filmstrip/gallery.
- Rating, color-label and flag indicators.
- Filtering by rating, color label, flag, version type, edit status, export status, file format, orientation, filename, camera and lens.
- Detail/Quick filter modes.
- Sorting and quick actions.

### Right control panel
Official desktop editing modules:
1. **Color Adjustments**
2. **Portrait Retouching**
3. **Background Adjustments**
4. **Clothing & Accessories Adjustments**
5. **Crop & Rotate**

Panel behavior:
- Slider + numeric input.
- Solo Mode.
- Collapse All.
- Expand All.
- Hold group eye icon to temporarily bypass a feature group.
- Hold a single slider/effect to compare only that effect.

### Favorites
- Top-level modules, submodules and individual sliders can be favorited.
- Drag/drop reorder.
- Favorites reference the original controls rather than copying state.
- Favorites stay fixed while image-specific values update.

---

# 2. Library and project workspace

### Project management
- Projects list.
- Related folders.
- Folder/subfolder management.
- Collections without duplicating original files.
- Bin / restore / permanent delete.

### Gallery views
- Grid.
- Edit.
- Loupe / large image.
- Survey mode, up to 12 images side-by-side.
- Reference/comparison workflows.

### Metadata
- EXIF and IPTC viewing/editing.
- Batch metadata edit.
- XMP sidecar compatibility.
- Folder refresh reads changes made by other software.

### Culling / organization
- Manual culling.
- AI-assisted culling where available.
- Ratings, flags and color labels.
- Photo clustering / face-oriented organization documented in recent releases.

### Presets and sync
- Recommended presets.
- Team presets.
- My Presets.
- Sync selectable groups:
  - AI Color Adjustment
  - Color Adjustment
  - Portrait Adjustment
  - Background Adjustment
  - Clothing & Accessories
  - Crop & Rotate

### Slideshow
- Background music.
- Background image/color.
- Intro/title page.
- Preview and export.

---

# 3. Color Adjustments

## AI Style / AI color

### AI Color Looks
- One-click AI-generated looks.
- Public categories documented by Evoto include Camera Simulation, Gentle, Tone, Film, Vibe, Trendy and B&W.

### AI Color Match
- Reference-image based style transfer.
- Harmonizes exposure, temperature, tone and local details.
- Semantic recognition is documented for local regions including skin, hair, clothing and environmental regions.
- Designed for batch/set consistency.

### Multi-Image Color Consistency
- Applies AI consistency across selected images in the same scene.
- Evoto recommends applying AI Style first, Auto Color Corrections next, and Multi-Image Color Consistency afterward.

### Background Color Consistency
- Documented in V7.1.5 release notes for matching background colors across selected images.

## Masking
Mask types documented by Evoto:
- Person Mask.
- Pet Mask.
- Background Mask.
- Custom Mask.

Local adjustment families:
- Basic.
- Curves.
- HSL.
- Color Grading.
- Detail / sharpening where documented.

Mask workflow:
- AI/manual masks appear in Quick Access.
- Brush add/remove refinement.
- Visibility toggle excludes hidden masks from export.
- Sync can Replace Existing Masks or Add to Existing Masks.

## Conventional color controls
TBRetoch must provide a complete non-destructive color stack rather than a few approximate sliders:
- Temperature / Tint.
- Exposure / Contrast.
- Highlights / Shadows.
- Whites / Blacks.
- Vibrance / Saturation.
- Curves.
- 8-band HSL: Red, Orange, Yellow, Green, Cyan/Aqua, Blue, Purple, Magenta.
- Color Grading.
- Sharpen/detail.
- Noise reduction / AI Denoise where applicable.
- Grain.
- Lens/transform corrections.
- Glow Effect: White Mist, Black Mist, Halation.
- Post-Crop Vignetting.

---

# 4. Portrait Retouching

TBRetoch must use semantic face/skin/body analysis. Approximate ellipses painted over eyes, lips or cheeks do not count as parity.

## Blemish / cleanup targets documented by Evoto
- Freckle & acne / blemish removal.
- Eye bags and lower-eyelid protection.
- Dark circles.
- Face shine reduction.
- Glasses glare removal + Strong Glare Removal.
- Nostril Cleanup.
- Lip wrinkle/flakes cleanup.
- Double chin / jawline cleanup.
- Multiple wrinkle zones.
- Body blemish cleanup.
- Stretch marks.
- Tan-line removal.
- Hand retouching.
- Manual tuning / refinement brush.

## Skin retouching

Facial skin:
- Even with Dodge & Burn.
- Sculpt with Dodge & Burn: facial features and facial contours.
- Textured smoothing.
- Frequency separation.
- Skin softening.
- Skin texture controls.
- Face fullness/radiance controls documented in current skin-retouching documentation.

Body skin:
- Body-skin smoothing/evening.
- Frequency separation.
- Body complexion unification.
- Collarbone enhancement.
- Skin-tone selection and harmonization.

## Facial reshape
TBRetoch target controls include the documented face geometry families:
- Head pose and symmetry.
- Face / temple / cheekbone / jaw.
- Face size, V-shape, jaw length, face width.
- Hairline / forehead.
- Philtrum / middle / lower facial sections.
- Chin geometry.
- Eyebrow geometry.
- Eye geometry.
- Nose geometry.
- Mouth geometry.
- Linked/unlinked left/right behavior where Evoto provides it.

## Eyes
- Eye brightness.
- Iris and eye-white controls.
- Reflection / iris flare.
- Red-vein cleanup.
- Eye-white cleanse.
- Red-eye removal.
- Eye symmetry correction.
- Catchlights.
- Direct-gaze / iris-correction family where documented.

## Teeth
- Teeth whitening/brightness.
- Teeth flaw cleanup.
- Edge/alignment correction where supported.

## Makeup
- Highlight.
- Contour.
- Eyebrow makeup.
- Eye makeup amount/saturation/brightness/dimensionality.
- Lip makeup amount/saturation/brightness/dimensionality.
- Makeup presets/components.

Current documented PC makeup breadth includes preset/component families for eyebrow, eyeshadow, eyelashes, eyeliner, contacts, blush, lipstick, contour and face decorations; several controls support linked or independent left/right adjustment.

## Hair
- Smooth Hair.
- Hair Shine.
- Hair color consistency/change.
- Part-line/sparse-area correction.
- Stray-hair removal.
- White-hair correction.
- Manual refinement.

## Full Body Reshape
Evoto documents AI reshape based on 3D skeleton/posture recognition. TBRetoch parity therefore requires landmarks/skeleton-based deformation, not rectangle scaling.

Target controls:
- AI Reshape.
- Smooth Physique.
- Head.
- Overall Body.
- Belly Slimming.
- Height.
- Upper-body length.
- Neck width/length.
- Arms.
- Chest/breast area where supported.
- Waist.
- Hips.
- Thigh/calf width.
- Leg length.
- Group-photo safety behavior.

---

# 5. Background Adjustments

## Solid backdrop refinement
- Distractions Removal.
- Clean Backdrop.
- Smart Removal.
- Unify Lighting.
- Color Banding Removal.
- Background Enhancement.
- Manual tuning brush for including/excluding affected regions.

## Matte Refinement
V7.3 adds Black & White Edge Removal to reduce black/white fringes at subject edges.

## Background replacement / set design
- Background removal/cutout.
- Transparent/white/black/custom backgrounds.
- AI Background Fusion.
- AI Set Design.
- **Batch AI Set Design** in V7.3.
- Floor Reflection.
- Subject placement / scaling / scene blending as required by the replacement workflow.
- Preserved-area modes documented for subject, subject+related objects, and broader object preservation.
- Fill-region and fill-mode controls.
- Edge adjustment, opacity, size, horizontal/vertical positioning.

## Sky Replacement
Documented controls:
- Sky selection/custom sky.
- Vertical/horizontal position.
- Flip.
- Edge Transition.
- Temperature.
- Tint.
- Saturation.
- Brightness.
- Sky Blur.
- Opacity.
- Scenery Color matching.
- Human Color matching.
- Water Reflection.
- Water Blur.
- Smart edge/manual brush refinement.

## Depth / blur
- Lens Blur.
- Background blur with subject-aware masking.

---

# 6. Clothing & Accessories

This must be a first-class module because Evoto exposes it as a first-class edit module.

Documented public features include:
- De-wrinkle Clothing.
  - Fine wrinkles.
  - Coarse wrinkles.
- De-Blemish Clothing for lint/dust/flakes.
- Clothing Edge Smoothing.
- Clothing Edge Refinement/manual tuning.
- Nearby-object protection.
- Shoe editing.
- Clothing extraction.
- Clothing color change.

---

# 7. Editing toolkit / manual tools

Public features include:
- Deblur / blur removal.
- Shadow removal.
- AI Magic Eraser / object removal.
- AI lighting effects.
- Resize.
- Rotate.
- Brighten / darken.
- Flip.
- RAW editing.
- AI enhancer.
- Old-photo restoration.
- RAW converter.
- Distortion correction.
- Crop.
- Healing / repair style tools where documented in Support.

---

# 8. Pet Retouching

- Pet masks.
- Pet-specific color adjustment through masking.
- Stray-fur cleanup is publicly listed as a pet-retouch feature.

---

# 9. Import, RAW and Lightroom workflow

Evoto documents:
- JPEG, TIFF, PNG and most camera RAW formats.
- Max 15,000 images in one import session.
- No stated project-total image limit.
- Current hard input limits: image file <= 1 GB and dimensions <= 12,000 x 12,000 px.
- Lightroom Classic round-trip support and basic color-parameter synchronization.
- Auto Import & Export / Hot Folder workflows.
- Integrity wait/check behavior for hot folders.
- Tethered-shooting workflow, including optional camera auto-connect.

TBRetoch internal target:
- libraw/rawspeed-class RAW decode path.
- ICC/color-managed preview/export.
- Hot-folder watcher.
- No destructive overwrite of original source pixels.

---

# 10. Export

A previous TBRetoch build incorrectly exported the reduced preview canvas. This is prohibited by the V0.5 architecture.

Evoto documents:
- Quick Export.
- Custom Export.
- Export with previous settings.
- Lightroom Catalog export when the project is a catalog workflow.
- Multiple export presets can be applied simultaneously, producing multiple outputs per source image.
- Original-format behavior.
- JPG 8-bit.
- TIFF 8-bit / 16-bit.
- PNG.
- JPG quality percentage plus Low / Medium / High / Best presets.
- Optional target file-size limit.
- Resize modes including percentage, width/height and long/short edge families.
- Resolution in PPI / PPC.
- Output sharpening:
  - Screen Low / Standard / High
  - Print Low / Standard / High
  - None
- Watermark controls including rotation, size, opacity and position.
- Metadata options.
- Max simultaneous exports automatically selected by hardware with user adjustment.

**Required TBRetoch behavior:** export always re-renders from the original-resolution source with the non-destructive recipe. Preview pixels are never used as final output.

---

# 11. Performance behavior to reproduce

Evoto explicitly documents separate controls for preview and export performance:

### Preview
- Portrait Effect Rendering Acceleration.
- Color Effect Rendering Acceleration.
- Preview memory optimization.
- Configurable preview size.
- Edited vs original thumbnails.
- Preview effect preloading for adjacent images.

### Export
- Portrait Effect Rendering Acceleration for export.
- Export memory optimization.
- Max simultaneous exports based on hardware.
- Effects are applied again during export for maximum quality.

### Group portrait optimization
- Optional faster processing for approximately 15+ subjects, with a documented speed/precision trade-off.

### TBRetoch implementation requirement
- UI thread does not run image-pixel loops or neural inference.
- GPU preview renderer.
- Asynchronous AI workers.
- CPU worker pool for RAW decode, preprocessing and encode.
- Cached masks, landmarks and semantic features.
- Progressive preview quality while dragging; full-quality settle after release.
- Original-resolution export pipeline.

GPU path target:
- DirectML for broad Windows GPU support.
- CUDA/TensorRT where a supported NVIDIA configuration is present.
- CPU fallback.

The app should display the active backend truthfully.

---

# 12. TBRetoch branding and internal-use decisions

- Product name: **TBRetoch**.
- Logo: **TB**.
- Theme: dark UI with TB purple accents.
- Internal/team build: **no credits, no export quota, no License Center**.
- Offline editing after local model installation.
- No Evoto logos, icons, artwork or proprietary visual assets.
- No Evoto model extraction or proprietary weight reuse.

---

# 13. Definition of parity acceptance

A feature does not count because a label or slider exists. It counts only if:

1. Correct semantic target is detected.
2. Adjustment is reversible and non-destructive.
3. Neighboring/background areas are protected.
4. Preview remains interactive while the control is dragged.
5. Full-resolution export reproduces the edit from original pixels.
6. Batch sync re-detects semantic regions independently per photo.
7. Multi-person edits can target the correct person where the Evoto feature supports it.
8. Left/right linked controls become independent when unlinked.
9. Offline behavior is tested with networking disabled after models are installed.
10. GPU and CPU fallback paths are both smoke-tested.
11. Before/After works at image, feature-group and single-effect level where applicable.
12. Image quality is evaluated at 100% and export file dimensions are verified against the original.

---

# 14. Source baseline

Primary Evoto pages used in this audit:
- Official Download page.
- Official Release Notes.
- Official Features catalog.
- Support: Edit Workspace Overview.
- Support: Library Workspace Overview.
- Support: System Settings / Performance.
- Support: Import & Export.
- Support: Export Quality.
- Support: Color Adjustments.
- Support: AI Color Match.
- Support: Masking.
- Support: Skin Retouching.
- Support: Facial Reshape.
- Support: Eyes / Makeup / Hair / Teeth.
- Support: Full Body Reshape.
- Support: Background Adjustments / Sky Replacement.
- Support: Clothing & Accessories.
- Support: Auto Import & Export / tethering.
- Direct static inspection of the user-provided official Evoto bootstrap installer.

This document supersedes earlier TBRetoch parity notes that incorrectly treated an unverified future version as the current Evoto release.