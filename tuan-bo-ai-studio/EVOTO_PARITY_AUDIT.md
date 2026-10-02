# TBRetoch V0.5 — Evoto Desktop parity audit

Audit date: 2026-10-02

Goal: rebuild the editing workflow and feature surface for internal/offline use without copying Evoto proprietary code, assets, model weights, branding, or exact visual assets. TBRetoch keeps its own TB branding and purple palette, while matching the workflow depth and control coverage users expect from Evoto.

## Official-version baseline used for this audit
- Evoto official Download page currently exposes a Windows stable build in the 7.3.x line depending on locale/cache.
- Evoto official Release Notes list **V8.0.0 (2026-09-22)** as the current key release.
- Therefore TBRetoch parity target is **Evoto 8.0 feature surface**, not only the older 7.3 desktop feature set.
- Official sources used: Evoto Download, Release Notes, Edit Workspace Overview, Library Workspace Overview, Color Adjustment Feature Modules, AI Color Match, Masking, Portrait Retouching, Facial Reshape, Full Body Reshape, AI Lab, Culling & Organization, Workflow Templates, Import & Export.

## Product shell / workspace
- Project/library workspace, import, file management, recent projects.
- Edit toolbar: Back to Project, Add Images, Undo, Redo, Manual Tools, Search.
- Floating widget: Presets, Masking, History.
- Preview: zoom, pan, fit, synchronized pan/zoom in compare view.
- View modes: single preview, Reference View, Before/After comparison, real-time color vs all-effects comparison, full screen.
- Canvas background choices: Default / Black / White / Dark Gray / Medium Gray / Light Gray.
- Filmstrip/gallery, multi-select, ratings/labels/filtering, virtual copies.
- Bottom gallery is resizable and indicators collapse when compressed.
- Copy/Paste/Sync effects with selectable effect groups.
- Global history and separate tool history for Liquify/Healing.
- Preview configuration: sRGB/Adobe RGB, preview size up to 4000 px, optional full-size real-time color preview, thumbnail mode, effect preloading.
- Favorites panel: user can pin top-level modules, submodules and individual sliders; drag/drop reorder; favorites mirror the original control values.

## Library / project workspace
- Projects list and sorting.
- Related Folders, subfolders, move/rename/delete.
- Collections without duplicating source files.
- Bin / restore / permanent delete.
- Thumbnail gallery with tagging/filtering.
- Filters: star rating, color label, flag, version type, edit status, export status, file format, orientation/aspect, filename, camera, lens.
- Sort: capture time, edit time, rating, file type, size, name.
- View modes: Single, Comparison, Reference.
- Quick Access panel: Culling, Presets, Sync, Metadata.
- Sync groups: AI Color, Color, Portrait, Background, Clothing & Accessories, Crop & Rotate.

## Color
### Histogram
- RGB histogram.

### Profiles / AI Looks
- Standard profile and LUT-based custom profiles.
- B&W profile mode with B&W-specific controls.
- AI Color Looks with categories such as Camera Simulation, Gentle, Tone, Film, Vibe, Trendy and B&W.
- Camera Simulation examples documented by Evoto include Fuji NC, Fuji NN, Ricoh+, Kodak Ultramax 400.
- Look intensity slider and preset support.

### Basic
- White Balance: Temperature, Tint.
- Exposure / Contrast / Highlights / Shadows / Whites / Blacks.
- Vibrance / Saturation.
- AI Auto White Balance and AI Auto Exposure.
- Auto Color Corrections one-click action.

### Curve
- Parametric curve.
- RGB/Luma/R/G/B curves.
- Multiple control points; on-image targeted curve adjustment.

### HSL
8 bands: Red, Orange, Yellow, Green, Aqua/Cyan, Blue, Purple, Magenta.
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
- Detail Enhance.

### Creative effects
- Glow Effect: White Mist, Black Mist, Halation.
- Post-Crop Vignetting.
- Grain.
- Lens Correction.
- Transform Correction.

### AI Color Match
- Reference image selection from local JPG/TIFF/PNG or current preview.
- Recommended references and reusable My Looks/reference management.
- Quick Mode and Control Mode.
- Full-image controls: Amount, Tone, Color.
- Semantic recognition for Face/Body Skin, Hair, Eyes, Lips, Teeth, Clothes, Background/local regions.
- Per-mask tone/color intensity.
- Linked/unlinked mask adjustments.
- Control Mode exposes generated adjustments back onto normal sliders and creates local masks for transparent fine tuning.
- Batch sync and preset save.
- Multi-Image Color Consistency for same-scene batches.
- Background Color Consistency for studio/headshot/school workflows.

## Masking
- Person mask with per-person selection.
- Pet mask.
- Background mask.
- Custom mask.
- Person subregions: facial skin / body skin / neck / all skin, hair, eyes, lips, teeth, clothes and other local regions.
- Manual brush add/subtract/refine.
- Local Basic / Curve / HSL / Color Grading / Detail adjustments applied nondestructively.
- Masks appear in the floating Quick Access widget.

## Portrait — person detection
- Multi-face recognition.
- Character/person tags and per-person edits.
- Face frame visualization.
- Add Face and Delete Face manually when automatic detection fails.
- Attribute correction when age/gender classification is wrong.
- Gender/age category handling including Male, Female, Child, Senior/Infant where supported.
- Batch sync per person across project images.

## Portrait — blemish removal / cleanup
- Freckle & Acne Removal.
- Eye Bags with Lower Eyelid Protection.
- Dark Circles.
- Reduce Face Shine.
- Remove Glasses Glare and Strong Glare Removal.
- Nostril Cleanup.
- Lip Wrinkles & Flakes.
- Double Chin / Jawline cleanup.
- Beard Protection.
- Wrinkle zones: Forehead, Eleven Lines, Eye Wrinkles, Nasal Wrinkle, Cheek Wrinkle, Marionette Lines, Perioral Wrinkle, Neck Wrinkle.
- Body Blemish.
- Infant Body Blemish.
- Remove Tan Lines.
- Hand/body vein cleanup.
- Armpit Touch-up.
- Stomach Stretch Marks + Pregnancy Line.
- Stretch Marks.
- Tattoos via manual masking.
- Manual tuning pen add/erase and original/mask view.
- Face Shadow Removal (Evoto 8.0).

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
- AI Body Complexion / complexion unification and eyedropper-based target.
- Collarbone enhancement.

## Facial reshape
### Head pose / symmetry
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
- Facial Fullness
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
- Length/Height
- Horizontal position
- Nose Bridge
- Width / Ala
- Nose Tip

### Mouth
- Size
- Width
- Vertical
- Horizontal
- Tilt
- M-shaped/Cupid lips
- Upper Lip
- Lower Lip

## Eyes — appearance
- Eye Brightness.
- Iris / Eye Whites / Eye Reflection / Iris Flare.
- Remove Glasses Glare / Strong Glare Removal.
- Red Vein Removal.
- Eye White Cleanse.
- Red Eye Removal.
- Eye Symmetry, Eye Level, Eye Balance.
- Catchlights: multiple styles, intensity and movable placement.
- AI Iris Correction / Direct Gaze with per-eye horizontal/vertical position and size where supported.
- Manual tuning pen.

## Facial expression
- Gummy Smile / gum exposure adjustment.
- Gentle Smile.
- Smile generation/management.
- Smirk/asymmetry correction where supported.

## Teeth
- Teeth Flaws Removal: braces, stains, gaps.
- Fix Teeth Edge.
- Teeth Whitening: Brightness, Desaturation.
- Teeth Alignment.
- Pretty Teeth / generated corrective set where appropriate.

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

## Hands / clothing / accessories / pets
- Hand beautification and vein cleanup.
- Clothing wrinkle removal.
- Clothing beautification options by subject attributes where supported.
- Clothing & Accessories must be a first-class feature module, not hidden under Portrait.
- Pet retouching module and Pet Masks.
- Pet leash removal / stray fur cleanup where supported.

## Full body reshape
- AI Reshape [-100,+100 class behavior].
- Smooth Physique.
- Head size.
- Body width/shape.
- Belly Slimming.
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
- Liquify Background Repair after geometry editing (Evoto 8.0 / AI Lab).

## Background
### Background cleanup
- Distractions Removal.
- Clean Backdrop.
- Smart Removal.
- Unify Lighting.
- Color Banding Removal.
- Background Enhancement.
- Matte Refinement / Black & White Edge Removal.

### Background replacement / AI Set Design / Fusion
- Subject Only vs Subject + Connected Objects.
- Official/custom background assets.
- Foreground layers and order.
- Subject reposition / scale / rotate / flip H/V.
- Character Lighting to blend subject with scene.
- Manual cutout refinement.
- AI Background Fusion.
- AI Set Design with batch processing.
- Floor Reflection / Ground Reflection.
- Grass Fill.

### Sky replacement
- AI Sky Replacement.
- Sky selection/replacement.
- Rotate/Flip, Angle, Sky Gradient.
- Edge Transition.
- Temperature, Tint, Saturation, Brightness.
- Sky Blur, Opacity.
- Scenery Color match.
- Human Color match.
- Water Reflection and Water Blur.

### Lens blur / depth
- Subject-aware focus.
- Lens Blur.
- Blur/bokeh controls and manual refinement.

## AI Lab / generative / restore / manual tools
- People Removal.
- Smart Removal.
- Healing/repair brush.
- Spot Healing / Patch / Clone Stamp.
- Liquify with dedicated history.
- Liquify Background Repair.
- Old Photo Restoration, optional colorization, 2K/4K output.
- Generative Expand (Evoto 8.0).
- Perfect Shot where available.
- Search tool / unified search.
- Crop with AI face positioning.
- Rotate with AI horizontal correction and angle slider.

## Presets / Asset Hub / batch workflow
- Recommended, Personal and Team presets.
- Save, import, group and manage presets.
- Asset Hub (Evoto 8.0): presets, backgrounds and creator/community assets.
- Batch sync selected effect groups.
- Sync popup frequency preference.
- Workflow templates for repeated import → culling → apply effect → export → deliver sequences.
- Workflow templates documented for wedding, portrait, headshot, school, family, baby, fashion and general workflows.
- Workflow run state/progress retained separately from reusable recipe.
- Import source options should include local folder/project; Lightroom Classic compatibility can be a later optional integration.

## Culling / organization
- Smart Culling at project level.
- Smart Photo Analysis for blur, closed eyes, exposure, face clusters and other attributes.
- Quick vs Custom culling preferences.
- Event-type-aware culling configuration.
- Faces and Photo Cluster views.
- Story Groups / Stories and Segments (Evoto 8.0).
- Ratings, flags, color labels, filters and sorting.
- Metadata panel.
- Virtual copies.
- Slideshow and Full Screen View.
- Hot Folder import safeguards.
- Tethered shooting / wired and wireless import are later optional modules.

## Export — must be full-resolution, never preview-canvas export
- Export Quick / Custom / Previous Settings.
- Multiple export presets simultaneously.
- Effect Preset at export; support separate outputs for multiple effect presets.
- Original Filename / Preset Name / Custom Text naming tokens.
- Export destination: desktop / folder / original folder and optional integrations.
- Size modes: Percentage, Width & Height, Dimensions, Long Edge, Short Edge.
- Resolution: PPI / PPC.
- Formats: Original, JPEG 8-bit, TIFF 8/16-bit, PNG.
- RAW original-format request resolves to JPEG once edits are applied; alpha images resolve to PNG.
- JPEG quality presets + 0–100 slider.
- Optional file-size target/limit.
- Output sharpening: Screen Low/Standard/High, Print Low/Standard/High, None.
- Watermark: rotation, size, opacity, position.
- Metadata retention choices.
- Configurable max simultaneous exports based on hardware.
- Export must reprocess original pixels at source resolution.

## TBRetoch internal/offline architecture requirements
- No license/credit system for this internal/team build.
- No network required after installation/model deployment.
- Preview render graph separate from export render graph.
- Preview: proxy pyramid / mip levels up to configurable 4000px, GPU-first rendering, progressive quality while slider is moving, full-quality settle after release.
- Pan/zoom must be matrix-based, independent of image pixels; mouse drag / wheel / keyboard shortcuts.
- Original pixels remain immutable; edits stored as nondestructive recipe.
- Export: original resolution, ICC-aware, metadata-preserving, batch queue.
- CPU: multithreaded RAW decode, JPEG/TIFF/PNG encode, preprocessing, model fallback inference.
- GPU: DirectML on broad Windows GPUs; CUDA/TensorRT where NVIDIA runtime permits; CPU fallback.
- Separate semantic models: face detection + landmarks/mesh; human pose/3D skeleton; portrait parsing; hair/skin/clothes/background parsing; matting; eye/iris landmarks; blemish/skin restoration; background/depth/relight.
- Cache masks/landmarks/features per image and invalidate only dependent stages.
- Async worker pool; UI thread never runs pixel loops or model inference.
- Do not automatically analyze every newly selected photo at full resolution; analysis is queued/cached and preview stays interactive.

## Definition of parity acceptance
A control is not considered implemented because a slider exists. It only counts when:
1. The intended target region is detected correctly.
2. Slider response is continuous and reversible.
3. Background/neighboring geometry is protected where applicable.
4. Preview remains interactive while dragging.
5. Export reproduces the full-resolution edit rather than exporting the preview.
6. Batch sync re-detects semantics for each image rather than reusing coordinates.
7. Multi-person images allow independent person targeting where the feature supports it.
8. Left/right linked controls actually unlink into independent semantic regions.
9. Offline mode is tested with networking disabled after model deployment.
10. GPU path and CPU fallback are both validated in automated smoke tests.
