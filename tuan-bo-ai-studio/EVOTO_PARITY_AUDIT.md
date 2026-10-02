# TBRetoch V0.5 — Evoto Desktop parity audit

Goal: rebuild the editing workflow and feature surface for internal/offline use without copying Evoto proprietary code, assets, model weights, branding, or exact visual assets.

## Product shell / workspace
- Project/library workspace, import, file management, recent projects.
- Edit toolbar: Back to Project, Add Images, Undo, Redo, Manual Tools, Search.
- Floating widget: Presets, Masking, History.
- Preview: zoom, pan, fit, synchronized pan/zoom in compare view.
- View modes: single preview, reference view, before/after split, real-time color vs all-effects comparison.
- Filmstrip/gallery, multi-select, ratings/labels/filtering, virtual copies.
- Copy/Paste/Sync effects with selectable effect groups.
- Global history and separate tool history for Liquify/Healing.
- Preview configuration: sRGB/Adobe RGB, preview size up to 4000 px, optional full-size real-time color preview, thumbnail mode, effect preloading.

## Color
### Histogram
- RGB histogram.

### Filters / AI Looks
- Filter/Look browser with intensity slider.
- Preset/AI Look support.

### Basic
- White Balance: Temperature, Tint.
- Exposure / Contrast / Highlights / Shadows / Whites / Blacks.
- Vibrance / Saturation.

### Curve
- Parametric curve.
- RGB/Luma/R/G/B curves.
- Up to multiple control points; on-image targeted curve adjustment.

### HSL
8 bands: Red, Orange, Yellow, Green, Aqua, Blue, Purple, Magenta.
- Hue
- Saturation
- Luminance
- On-image HSL picker.

### Color Grading / Calibration
- Shadow / Midtone / Highlight grading.
- RGB primary hue/saturation calibration.

### Detail
- Sharpen: Amount, Radius, Detail.
- Noise Reduction: Amount, Detail, Contrast.

### AI Color Match
- Reference image selection.
- Quick Mode and Control Mode.
- Semantic masks for Face/Body Skin, Hair, Eyes, Lips, Teeth, Clothes, Background/local regions.
- Per-mask tone/color intensity.
- Linked/unlinked mask adjustments.
- Batch sync and preset save.
- Background Color Consistency for same-scene batch matching.

## Masking
- Person masks and per-person selection.
- Skin: facial skin / body skin / neck / all skin.
- Hair, eyes, lips, teeth, clothes, background and custom regions.
- Manual brush add/subtract/refine.
- Local color adjustments applied nondestructively.

## Portrait — person detection
- Multi-face recognition.
- Character/person tags and per-person edits.
- Gender/age category handling including Child and Infant.
- Batch sync per person across project images.

## Portrait — blemish removal
- Freckle & Acne Removal.
- Eye Bags with Lower Eyelid Protection.
- Dark Circles.
- Reduce Face Shine.
- Remove Glasses Glare.
- Nostril Cleanup.
- Lip Wrinkles & Flakes.
- Double Chin.
- Beard Protection.
- Wrinkle zones: Forehead, Eleven Lines, Eye Wrinkles, Nasal Wrinkle, Cheek Wrinkle, Marionette Lines, Perioral Wrinkle, Neck Wrinkle.
- Body Blemish.
- Infant Body Blemish.
- Feet Vein / hand/body vein-related cleanup where supported.
- Armpit Touch-up.
- Stomach Stretch Marks + Pregnancy Line.
- Stretch Marks.
- Tattoos via manual masking.
- Manual tuning pen add/erase and original/mask view.

## Portrait — skin retouching
### Facial skin
- Even with Dodge & Burn.
- Sculpt with Dodge & Burn: Facial Features, Facial Contours.
- Textured Smoothing.
- Frequency Separation: High Frequency, Low Frequency.
- Skin Softening.
- Fullness/radiance style controls for face zones.

### Body skin
- Smooth/moist body skin.
- Frequency Separation for body.
- Complexion unification and eyedropper-based complexion target.
- Collarbone enhancement.

## Facial reshape
### Head pose
- Up/Down [-100,+100]
- Left/Right [-100,+100]
- Tilt [-100,+100]
- Facial Symmetry [0,100]
- Upper Body Symmetry [0,100]

### Face shape
- Face
- Temple
- Cheekbone
- Jaw
- Face Size
- V-Shape, linked/unlinked sides
- Jaw Length
- Face Width
- Hairline
- Forehead Height L/R
- Forehead Width L/R
- Philtrum
- Middle Section
- Lower Section
- Taper Chin
- Chin Length
- Chin Shape

### Eyebrows
- Thickness
- Distance
- Tilt
- Arch Height
- Position
- linked/unlinked left/right

### Eyes geometry
- Full Eye Size
- Eyeball Size
- Height
- Width
- Distance
- Inner Corner
- Outer Corner
- Tilt
- Position
- linked/unlinked left/right

### Nose
- Size
- Length
- Horizontal
- Nose Bridge
- Width
- Nose Tip

### Mouth
- Size
- Width
- Vertical
- Horizontal
- Tilt
- M-shaped Lips
- Upper Lip
- Lower Lip

## Eyes — appearance
- Eye Brightness.
- Subcontrols: Iris (0–100, default 80), Eye Whites (0–100, default 80), Eye Reflection (0–100, default 40), Iris Flare (0–100, default 0).
- Remove Glasses Glare.
- Red Vein Removal.
- Eye White Cleanse.
- Red Eye Removal.
- Eye Symmetry, Eye Level, Eye Balance.
- Catchlights: multiple styles, intensity, movable placement.
- AI Iris Correction: Direct Gaze, horizontal/vertical position and size per eye.
- Manual tuning pen.

## Facial expression
- Gum exposure adjustment.
- Smile/smirk management.
- Smile generation/management where available.

## Teeth
- Teeth Flaws Removal: braces, stains, gaps.
- Fix Teeth Edge.
- Teeth Whitening: Brightness, Desaturation.
- Teeth Alignment (multi-level correction).
- Pretty Teeth / full generated corrective set where appropriate.

## Makeup
### Basic
- Highlight.
- Contour.
- Eyebrow Makeup.
- Eye Makeup: Amount, Saturation, Brightness, Dimensionality.
- Lip Makeup: Amount, Saturation, Brightness, Dimensionality.

### Makeup suites / components
- Preset makeup suites with Amount.
- Eyebrow styles.
- Eyeshadow styles.
- Eyelash styles, linked/unlinked.
- Eyeliner styles.
- Contacts.
- Blush styles.
- Lipstick colors + texture.
- Contour styles.
- Face decorations/freckle styles.

## Hair
- Hair Part Line / sparse-area fill.
- Top Hair Volume.
- Side Hair Volume.
- Hairline refinement.
- Stray Hairs Removal with zone controls.
- Smooth Hair.
- Hair Shine.
- Hair color consistency / color changes.
- Manual refinement where needed.

## Full body reshape
- AI Reshape [-100,+100 class behavior].
- Smooth Physique.
- Head size.
- Body width/shape.
- Height.
- Upper Body Length.
- Neck Width L/R and linked.
- Neck Length [-100,+100].
- Arms.
- Breasts/chest size.
- Waist Width L/R and linked.
- Waist Length.
- Hips L/R and linked.
- Leg Width: thighs/calves separate or linked.
- Leg Length with thigh/calf subcontrols.
- 3D skeleton/posture-aware protection and group-photo safety rules.

## Hands / clothing / full-body cleanup
- Hand beautification and vein cleanup.
- Clothing wrinkle removal.
- Clothing beautification options by subject attributes where supported.
- Pet leash removal / stray fur cleanup where supported.

## Background
### Background cleanup
- Distractions Removal.
- Clean Backdrop.
- Smart Removal.
- Unify Lighting.
- Color Banding Removal.
- Background Enhancement.

### Background replacement / AI Set Fusion
- Subject Only vs Subject + Connected Objects.
- Official/custom background assets.
- Foreground layers and order.
- Subject reposition / scale / rotate / flip H/V.
- Character Lighting to blend subject with scene.
- Manual cutout refinement.

### Sky replacement
- Sky selection/replacement.
- Rotate/Flip, Angle, Sky Gradient.
- Edge Transition.
- Temperature, Tint, Saturation, Brightness.
- Sky Blur, Opacity.
- Scenery Color match.
- Human Color match.
- Water Reflection and Water Blur.

### Lens blur / depth
- Subject-aware focus, blur/bokeh controls and manual refinement.

## AI Lab / manual tools
- People Removal.
- Smart Removal.
- Healing/repair brush.
- Liquify with dedicated history.
- Search tool / unified search.
- Crop with AI face positioning.
- Rotate with AI correction and angle slider.

## Presets / batch workflow
- Recommended, Personal and Team presets.
- Save, import, manage presets.
- Batch sync selected effect groups.
- Sync popup frequency preference.
- Workflow templates for repeated import → culling → edit → export sequences.

## Culling / library
- Smart Culling.
- Ratings, labels, filters, sorting.
- Metadata panel.
- Virtual copies.
- Hot Folder import safeguards.
- Tethered shooting / auto import where needed later.

## Export — must be full-resolution, never preview-canvas export
- Export Quick / Custom / Previous Settings.
- Multiple export presets simultaneously.
- Original Filename / Preset Name / Custom Text naming tokens.
- Size modes: Percentage, Width & Height, fixed dimensions, Long Edge, Short Edge.
- Resolution: PPI / PPC.
- Formats: Original, JPEG 8-bit, TIFF 8/16-bit, PNG.
- JPEG quality presets + percentage slider.
- Optional file-size target/limit.
- Output sharpening: Screen Low/Standard/High, Print Low/Standard/High, None.
- Watermark: rotation, size, opacity, position.
- Metadata retention choices.
- Configurable max simultaneous exports based on hardware.
- Export must reprocess original pixels at source resolution.

## TBRetoch internal/offline architecture requirements
- No license/credit system.
- No network required after installation/model deployment.
- Preview render graph separate from export render graph.
- Preview: proxy pyramid / mip levels up to configurable 4000px, GPU-first rendering, progressive quality while slider is moving, full-quality settle after release.
- Pan/zoom must be matrix-based, independent of image pixels; mouse drag / wheel / keyboard shortcuts.
- Original pixels remain immutable; edits stored as nondestructive recipe.
- Export: original resolution, ICC-aware, metadata-preserving, batch queue.
- CPU: multithreaded decoding/encoding, preprocessing, fallback inference.
- GPU: DirectML/CUDA/TensorRT where model/runtime license permits; CPU fallback.
- Separate semantic models: face detection + landmarks/mesh; human pose/3D skeleton; portrait parsing; hair/skin/clothes/background parsing; matting; eye/iris landmarks; blemish/skin restoration; background/relight.
- Cache masks/landmarks/features per image and invalidate only dependent stages.
- Async worker pool; UI thread never runs pixel loops or model inference.

## Definition of parity acceptance
A control is not considered implemented because a slider exists. It only counts when:
1. The intended target region is detected correctly.
2. Slider response is continuous and reversible.
3. Background/neighboring geometry is protected where applicable.
4. Preview remains interactive while dragging.
5. Export reproduces the full-resolution edit rather than exporting the preview.
6. Batch sync re-detects semantics for each image rather than reusing coordinates.
