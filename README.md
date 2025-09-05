# Font Icon Studio Pro (U++ 2025.1)

A modular U++ desktop app for composing **font-based icons** from two layers (A/B), previewing true pixel sizes (16–256), and saving icons into a persistent, browsable **Gallery**.

## Why two packages?
- **IconGallery** (separate package): a reusable, high-performance thumbnail grid (zoom 32→128 px, virtualization-ready, metadata overlays) for 10k+ items. We’ll use this across other tools (VFX shot bins, editorial slates, etc).
- **FontIconStudio** (main app): the TopWindow with 4 panes (Previews | Stage | Layer Tabs | Gallery), menu/toolbar stubs, and PNG export pipeline.

> Build the **IconGallery** first (with a tiny harness) to nail UX/perf. The main app scaffold lives in parallel and links the gallery as soon as it’s ready.

---
## Repo structure (Rough)

LICENSE
README.md
main.cpp
upp_font_icon_studio.upp
IconGalleryCtrl/
  IconGalleryCtrl.cpp
  IconGalleryCtrl.h
  IconGalleryCtrl.upp
include/
src/

---

## Build (TheIDE)
1. Open TheIDE → `Package organizer` → `Add nest` → choose repo root folder (so it sees `FontIconStudio/…`).
2. Select main package: **upp_font_icon_studio**.
3. Set configuration: `GUI` (default), U++ **2025.1** or newer.
4. Build & Run.

---

## Development plan (phased)

**Phase 0 — Scaffold (this commit)**
- TopWindow with 4 panes (left/center/right/bottom) sized like the HTML mockup.
- Simple top “toolbar bar” (frame container) with **Output size**, **Export PNG**, **Save** (stubs).
- No external deps beyond `Core` + `CtrlLib`.

**Phase 1 — IconGallery package**
- Implement `LibraryCtrl` (zoom 32–128, selection, reflow, cached preview, virtualization).
- `gallery_demo.cpp` loads 10k dummy items to prove perf.
- Integrate into `FontIconStudio` bottom pane.

**Phase 2 — Stage + Layers**
- `StageCtrl` (Painter-based rotation/opacity, checkerboard toggle).
- `LayerPanel` (Text/Codepoint/Font/Size/FG/BG/Opacity/Visible/Offsets/Rotate/Overlay).
- Export PNG (add `Painter`, `plugin/png` in `.upp`).
