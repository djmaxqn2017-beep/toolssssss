# Tuấn Bồ AI Studio

Windows desktop photo editor prototype with a dark-purple Tuấn Bồ interface and an offline-first architecture.

## V0.1 scope

Working in this milestone:
- Library import for JPG/JPEG/PNG/WebP
- Large preview and Before/After
- Non-destructive color settings (exposure, contrast, highlights, shadows, temperature, tint, saturation)
- Copy/Paste settings and local presets
- Basic local Color Match recipe from a reference image
- Single and batch export to JPG/PNG
- Project save/load
- Offline license validation with Ed25519 signatures
- Separate **Tuấn Bồ License Center** for issuing quota or timed licenses
- Model manifest and adapter architecture for later AI modules

Planned AI modules after V0.1:
- Portrait/skin parsing and retouch
- Smart masks
- Face/body landmarks and protected reshape
- Background matting
- Relighting
- Upscale/restoration
- Semantic AI Color Match

## Model policy

The product must not redistribute proprietary Evoto/Magimir weights or code. Only models/weights whose licenses are confirmed compatible with the intended commercial distribution should be bundled. The model manifest records candidate engines and license notes.

## Development

Studio:
```bash
cd studio
npm install
npm start
```

License Center:
```bash
cd license-center
npm install
npm start
```

## Windows installers

The GitHub Actions workflow `.github/workflows/tuan-bo-ai-studio-windows.yml` builds unsigned Windows NSIS installers for both apps. Code signing should be added before public commercial distribution.
