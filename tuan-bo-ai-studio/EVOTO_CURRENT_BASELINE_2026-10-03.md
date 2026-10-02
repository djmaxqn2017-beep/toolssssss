# Evoto current public baseline — 2026-10-03

This note supersedes only the **version-baseline paragraph** in `EVOTO_PARITY_AUDIT.md`. The feature inventory in that audit remains useful and V8 additions below are additive.

## What Evoto publicly shows today

Evoto's first-party pages are not fully synchronized by locale/cache:

- The Vietnamese Download page observed today still reports Windows **7.3.0-512**, updated 2026-08-06.
- The English Download page indexed recently reports **7.3.5-165**, updated 2026-09-08.
- Another localized Download page reports **7.3.5-185**, updated 2026-09-27.
- Evoto's first-party Release Notes currently list **V8.0.0 — September 22, 2026** as the newest major release.

Because the user's supplied installer cannot currently be resolved through the connected Drive mount, TBRetoch will target the **documented V8.0 feature surface** while recording the exact installer build separately once that binary becomes addressable.

## V8.0 additions documented by Evoto

- Asset Hub.
- Story Groups.
- Workflow Templates / reusable workflow sequences.
- Generative Expand.
- AI Sky Replacement in AI Lab.
- Grass Fill.
- Liquify Background Repair.
- Face Shadow Removal.
- Full Screen View.
- Home page shortcuts for Create Project, Tethered Shooting, Workflow, Asset Hub, Ask Me.
- Evoto AI Assistant and Unified Search for Projects, Features and Presets.

## TBRetoch rule

TBRetoch is a clean-room implementation for internal/team use:

- Match workflow depth and functional coverage, not Evoto proprietary implementation.
- TB branding and purple theme only.
- No Evoto source, private assets, model weights, license bypass, or extracted proprietary resources.
- AI processing remains local after model deployment.
- No credits/license system for the internal build.
- CPU + GPU pipeline with fallback and truthful backend reporting.
- Preview and final export are separate pipelines.
- An item is never marked implemented because a UI control exists; it must pass the parity acceptance criteria in `EVOTO_PARITY_AUDIT.md`.
