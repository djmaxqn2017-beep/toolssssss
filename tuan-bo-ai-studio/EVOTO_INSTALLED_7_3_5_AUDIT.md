# Evoto installed-build audit for TBRetoch

Source: metadata-only audit generated from a locally installed Evoto build. No Evoto binaries, proprietary model weights, or visual assets are copied into this repository.

## Verified installed build

- Evoto executable product version: **7.3.5.0**.
- Installed application footprint observed by audit: **1,814 files**, **2.55 GiB**.
- Executables: **12**; DLLs: **140**.
- Main process: `Evoto.exe` (~284 MB).
- Worker process: `Evoto-worker.exe` (~269 MB).
- Camera/tether process: `Evoto-camera-link.exe` (~25 MB).

## Desktop architecture evidence

- Native desktop shell uses **Qt 5.15.2** components including Qt Core/Gui/Widgets/QML/Quick/WebEngine/WebSockets.
- A distinct heavyweight worker process exists, supporting the design conclusion that expensive image/AI work should not run on the UI thread.
- Camera/tether support is separated into a dedicated process and ships vendor/camera SDK components (Canon, Sony, Fujifilm and gPhoto/PTP related files).
- FFmpeg libraries are bundled for media workflows. SQLite plugin support is present.
- The install contains a very large opaque resource bundle named `resources/pixcook` (~1.34 GiB). TBRetoch must **not** copy/decrypt/extract proprietary Evoto models from this bundle; it is only evidence that Evoto separates large processing resources from the UI shell.

## Installed effect-family inventory

The installation exposes **102 top-level effect/config families** by path name. These names are used only as interoperability/feature-surface evidence.

`AIReshape`, `AIShine`, `AISkinUnify`, `BeardProtect`, `BgBalance`, `BgClean`, `BgDeMoire`, `BgEnhance`, `BgFloorReflection`, `BgFloorReflectionPrepare`, `BgMistake`, `BgReplaceCustomColor`, `BgShadow`, `BgShadowRetain`, `BgShadowRetainPostProcess`, `BodyAcne`, `BodyDermabrasionHLF`, `BodyPortraiture`, `BodySkinUnify`, `BrightEye`, `CloseMouth`, `ClothesBlemishRemoval`, `ClothesCrease`, `ClothesEdgeSmooth`, `CollarBoneEnhance`, `CustomizeWaterMark`, `DeTattoo`, `Debug`, `Degreasing`, `DoubleChin`, `Edit`, `EyeRemoveRedBlood`, `EyeWhiten`, `EyelidGenerator`, `EyesSize`, `FaceAcne`, `FaceAcneBaby`, `FaceDermabrasionHLF`, `FacePlump`, `FacePortraiture`, `FaceShaping3D`, `FaceStereo`, `FaceSurgery`, `Filters`, `GlassReflectionRemoval`, `GrowthLine`, `HairColorAdjust`, `HairColorUnify`, `HairGloss`, `HairSeam`, `HairSmooth`, `Inpaint`, `IrisEditing`, `IrisEditingRedMask`, `LensBlur`, `LensCorrect`, `LipWrinkleRepair`, `Liquify`, `MakeupEnhance`, `MakeupProtect`, `Makeups`, `MatteRefinement`, `MatteRefinementMix`, `MultiplePeopleAISkinUnify`, `NeckLines`, `NeutralGray`, `NeutralGrayBodyEven`, `NeutralGrayDB`, `NeutralGrayHand`, `NoseFlaw`, `PerspectiveTransform`, `PetBrightEye`, `PoseRefine`, `PostCropVignetting`, `PursedLipsSmile`, `RealGrain`, `RedEyeReduction`, `RemoveBlueVeins`, `RemoveDoubleEyelid`, `RemoveEyeBag`, `RemoveGingiva`, `RemoveHair`, `RemovePetHair`, `RemoveStubble`, `RemoveTanLines`, `RemoveWhiteHair`, `RepairBreast`, `SkinAdjust`, `SkinLighten`, `SkinTexture`, `SkinUnifyColorPicker`, `SmileMouth`, `SmileMouth3D`, `StretchMark`, `TeethWhitenV3`, `ToothBeauty`, `ToothRepairing`, `WHairSkinCpu`, `Wrinkle`, `WrinkleV1`, `none-config.json`, `none-toning-config.json`

## Makeup asset/config surface

- `Suit`: 490 files
- `Lipstick`: 179 files
- `Eyelash`: 85 files
- `Eyemakeup`: 67 files
- `Catchlights`: 60 files
- `AnimalCatchlights`: 55 files
- `EyeLiner`: 47 files
- `Eyepupil`: 44 files
- `Silkworm`: 26 files
- `Blusher`: 24 files
- `Eyebrow`: 21 files
- `Contour`: 20 files
- `FacialDecoration`: 18 files

## Built-in guide/tutorial evidence

Installed tutorial media confirms first-class workflows including:

- `AIColorMatchedMode.mp4`
- `AICullingEntryGuide.mp4`
- `AICullingHoverGuide.mp4`
- `AIEnhance.mp4`
- `AiLabGuide.mp4`
- `AiScene.mp4`
- `AITrainLookGuide.mp4`
- `ai_backgroud_fusion_advert.mp4`
- `aI_enhance_advert.mp4`
- `ApplyColorMatchSecond.mp4`
- `ApplyColorMatchThird.mp4`
- `BellySlimming.mp4`
- `Bulk.mp4`
- `CloudLoadingPng.mp4`
- `ColorConsistency.mp4`
- `CreateProject.mp4`
- `DuplicateFilter.mp4`
- `eyeCompare.mp4`
- `filter.mp4`
- `InstantGalleryGuide.mp4`
- `libraryGuide.mp4`
- `lost_fat_advert.mp4`
- `NewLibraryGuided.mp4`
- `ObjectSelectionHoverGuide.mp4`
- `ObjectSelectionInteractiveHoverGuide.mp4`
- `ObjectSelectionQuickHoverGuide.mp4`
- `ObjectSelectionSegmentHoverGuide.mp4`
- `OldPhotoRestoration.mp4`
- `old_photo_advert.mp4`
- `PeopleRemove.mp4`
- `people_removal_advert.mp4`
- `PerfectShotGuide.mp4`
- `QuickRatingGuide.mp4`
- `SmartRemove.mp4`
- `SmartRemoveGuide.mp4`
- `smart_removal_advert.mp4`
- `StrongGlareRemoval.mp4`
- `strong_glare_removal_advert.mp4`
- `SyncToCloud.mp4`
- `UpLoadFirstV.mp4`

## TBRetoch implementation consequences

1. Replace the Electron/JavaScript pixel-loop prototype with a **native Qt/C++ shell** and a dedicated processing worker architecture.
2. Keep UI rendering, pan/zoom and interaction on the UI/GPU path; never run full-image CPU loops on slider events.
3. Maintain cached semantic outputs per image/person: face landmarks/mesh, body pose, skin, hair, eyes, lips, teeth, clothes, person/background mattes, depth, object masks.
4. Preview is a reduced non-destructive render; export always re-renders from the original-resolution source.
5. Use legal/open or internally trained models for TBRetoch. Do not reuse Evoto code, encrypted configs, proprietary models, icons, media, fonts, or branded assets.
6. Internal/team edition: no quota/license system; offline processing after local model installation.
7. Feature parity acceptance is behavioral: correct semantic target, reversible editing, interactive preview, full-res export, per-photo re-detection on sync, multi-person targeting, and CPU/GPU fallback.

## Priority implementation map

- **Foundation:** Library/project DB, import/RAW, responsive viewer, pan/zoom, undo/redo/history, presets, sync, export queue.
- **Color:** basic tone, curves, HSL, grading, masking, AI color match/consistency, lens/transform, grain/glow/vignette.
- **Portrait semantic stack:** face/skin/eyes/teeth/lips/hair/person IDs plus blemish, skin, reshape, makeup, hair and body families.
- **Background:** clean backdrop, matte refinement, replacement, fusion/set-design equivalent workflows, sky, shadow/reflection and lens blur.
- **Clothing:** wrinkle/blemish/lint, edge smoothing, extraction and color.
- **Manual tools:** healing/inpaint/object removal/liquify/crop/rotate/perspective.
- **Workflow:** culling, ratings/labels/flags, survey/reference, metadata/XMP, hot folder/tether, batch export.
