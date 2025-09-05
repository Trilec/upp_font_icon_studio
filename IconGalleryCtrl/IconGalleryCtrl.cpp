#include "IconGalleryCtrl.h"

static inline int ClampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// ---------------- Construction ----------------
IconGalleryCtrl::IconGalleryCtrl()
{
    BackPaint();

    // Add our scrollbars as a frame and wire the callback (no args)
    AddFrame(sb);
    sb.AutoHide();
    sb.NoBox();
    sb.SetLine(20); // wheel step
    sb.WhenScroll = [=] {
        scroll_y = sb.GetY();
        Refresh();
    };
}

// ---------------- Item add ----------------
int IconGalleryCtrl::Add(const String& name, const Image& img, Color tint) {
    IconGalleryItem it;
    it.name = name;
    it.seed = IsNull(tint) ? AutoColorFromText(name) : tint;

    if(!img.IsEmpty()) {
        it.src = img;
        it.status = ThumbStatus::Auto; // src takes precedence
    }

    items.Add(pick(it));
    Reflow(); Refresh();
    return items.GetCount() - 1;
}

void IconGalleryCtrl::AddDummy(const String& name) { Add(name); }

// ---------------- Attach / clear real image ----------------
bool IconGalleryCtrl::SetThumbFromFile(int index, const String& filepath) {
    if(index < 0 || index >= items.GetCount()) return false;
    Image m = StreamRaster::LoadFileAny(filepath);
    if(IsNull(m)) { SetThumbStatus(index, ThumbStatus::Missing); return false; }
    SetThumbImage(index, m);
    return true;
}

void IconGalleryCtrl::SetThumbImage(int index, const Image& img) {
    if(index < 0 || index >= items.GetCount()) return;
    auto& it = items[index];
    it.src = img;
    it.status = ThumbStatus::Auto;
    it.thumb_normal = Image();
    it.thumb_gray   = Image();
    Refresh();
}

void IconGalleryCtrl::ClearThumbImage(int index) {
    if(index < 0 || index >= items.GetCount()) return;
    auto& it = items[index];
    it.src = Image();
    it.thumb_normal = Image();
    it.thumb_gray   = Image();
    Refresh();
}

// ---------------- Status toggle ----------------
void IconGalleryCtrl::SetThumbStatus(int index, ThumbStatus s) {
    if(index < 0 || index >= items.GetCount()) return;
    auto& it = items[index];
    if(it.status == s) return;
    it.status = s;
    it.thumb_normal = Image();
    it.thumb_gray   = Image();
    Refresh();
}

// ---------------- Selection helpers ----------------
Vector<int> IconGalleryCtrl::GetSelection() const {
    Vector<int> out;
    for(int i = 0; i < items.GetCount(); ++i)
        if(items[i].selected) out.Add(i);
    return out;
}

void IconGalleryCtrl::SelectRange(int a, int b, bool additive) {
    if(a > b) Swap(a, b);
    if(!additive) for(auto& it : items) it.selected = false;
    for(int i = a; i <= b && i < items.GetCount(); ++i)
        items[i].selected = true;
}

// ---------------- Filtering ----------------
void IconGalleryCtrl::SetFiltered(int index, bool filtered_out) {
    if(index < 0 || index >= items.GetCount()) return;
    items[index].filtered_out = filtered_out;
    Refresh();
}

void IconGalleryCtrl::ClearFilterFlags() {
    for(auto& it : items) it.filtered_out = false;
    Refresh();
}

// ---------------- Zoom ----------------
void IconGalleryCtrl::SetZoomIndex(int zi) {
    zi = ClampInt(zi, 0, zoom_steps.GetCount() - 1);
    if(zi == zoom_i) return;
    zoom_i = zi;
    if(WhenZoom) WhenZoom(zoom_i);
    for(auto& it : items) { it.thumb_normal = Image(); it.thumb_gray = Image(); }
    Reflow(); Refresh();
}

// ---------------- Layout / Scrollbars sync ----------------
void IconGalleryCtrl::Reflow() {
    Size sz  = GetSize();
    int tile = zoom_steps[zoom_i];
    int boxW = tile + 2*pad;
    int boxH = tile + labelH + 2*pad;

    cols = max(1, (sz.cx - pad) / (boxW + pad));

    int rows = items.GetCount() ? ((items.GetCount() - 1) / cols + 1) : 0;
    content_h = pad + rows * (boxH + pad);

    int max_scroll = max(0, content_h - sz.cy);
    scroll_y = ClampInt(scroll_y, 0, max_scroll);

    // Tell ScrollBars: pos/page/total
    sb.Set(Point(0, scroll_y), sz, Size(sz.cx, content_h));
}

Rect IconGalleryCtrl::IndexRectNoScroll(int i) const {
    int tile = zoom_steps[zoom_i];
    int boxW = tile + 2*pad;
    int boxH = tile + labelH + 2*pad;
    int r = i / cols, c = i % cols;
    int x = pad + c * (boxW + pad);
    int y = pad + r * (boxH + pad);
    return RectC(x, y, boxW, boxH);
}

Rect IconGalleryCtrl::IndexRect(int i) const {
    Rect raw = IndexRectNoScroll(i);
    return RectC(raw.left, raw.top - scroll_y, raw.Width(), raw.Height());
}

// ---------------- Glyph builders ----------------
Image IconGalleryCtrl::MakePlaceholderGlyph(int tile, bool gray) {
    ImageBuffer ib(tile, tile);
    RGBA* p = ib.Begin();
    for(int i = 0; i < tile * tile; ++i, ++p) { p->r = p->g = p->b = 0; p->a = 255; }
    ib.End();
    BufferPainter bp(ib);
    Color a = Color(24,28,34), b = Color(18,22,27);
    int step = 8;
    for(int y = 0; y < tile; y += step)
        for(int x = 0; x < tile; x += step)
            bp.Rectangle(x, y, step, step).Fill(((x + y) / step) % 2 ? a : b);
    IconGalleryCtrl().DrawPlaceholderGlyph(bp, tile, gray);
    return ib;
}

Image IconGalleryCtrl::MakeMissingGlyph(int tile, bool gray) {
    ImageBuffer ib(tile, tile);
    RGBA* p = ib.Begin();
    for(int i = 0; i < tile * tile; ++i, ++p) { p->r = p->g = p->b = 0; p->a = 255; }
    ib.End();
    BufferPainter bp(ib);
    Color a = Color(24,28,34), b = Color(18,22,27);
    int step = 8;
    for(int y = 0; y < tile; y += step)
        for(int x = 0; x < tile; x += step)
            bp.Rectangle(x, y, step, step).Fill(((x + y) / step) % 2 ? a : b);
    IconGalleryCtrl().DrawMissingGlyph(bp, tile, gray);
    return ib;
}

const Image& IconGalleryCtrl::PlaceholderGlyph(int tile) {
    static VectorMap<int, Image> cache;
    int i = cache.Find(tile);
    if(i < 0) { cache.Add(tile, MakePlaceholderGlyph(tile, false)); i = cache.Find(tile); }
    return cache[i];
}
const Image& IconGalleryCtrl::MissingGlyph(int tile) {
    static VectorMap<int, Image> cache;
    int i = cache.Find(tile);
    if(i < 0) { cache.Add(tile, MakeMissingGlyph(tile, false)); i = cache.Find(tile); }
    return cache[i];
}

// Internal primitive draws
void IconGalleryCtrl::DrawPlaceholderGlyph(BufferPainter& p, int tile, bool gray) const {
    Color edge = gray ? SColorDisabled() : SColorText();
    int   m    = max(2, tile / 10);
    int   t    = max(1, tile / 16);
    int dash = max(2, tile / 12);
    int gap  = dash;

    for(int x = m; x < tile - m; x += dash + gap) {
        int w = min(dash, tile - m - x);
        p.Rectangle(x, m, w, t).Fill(edge);
        p.Rectangle(x, tile - m - t, w, t).Fill(edge);
    }
    for(int y = m; y < tile - m; y += dash + gap) {
        int h = min(dash, tile - m - y);
        p.Rectangle(m, y, t, h).Fill(edge);
        p.Rectangle(tile - m - t, y, t, h).Fill(edge);
    }
    int cx = tile / 2, cy = tile / 2, arm = (tile - 2*m) / 3;
    p.Rectangle(cx - t/2, cy - arm, t, 2*arm).Fill(edge);
    p.Rectangle(cx - arm, cy - t/2, 2*arm, t).Fill(edge);
}

void IconGalleryCtrl::DrawMissingGlyph(BufferPainter& p, int tile, bool gray) const {
    const Color warn = gray ? SColorDisabled() : Color(200, 60, 60);
    const int   m    = max(2, tile / 10);
    const int   t    = max(2, tile / 14);

    p.Rectangle(m, m,                tile - 2*m, t          ).Fill(warn);
    p.Rectangle(m, tile - m - t,     tile - 2*m, t          ).Fill(warn);
    p.Rectangle(m, m,                t,          tile - 2*m ).Fill(warn);
    p.Rectangle(tile - m - t, m,     t,          tile - 2*m ).Fill(warn);

    const int cx   = tile / 2;
    const int barh = (tile - 2*m) * 2 / 3;
    const int dot  = max(2, t);

    p.Rectangle(cx - t/2,
                m + (tile - 2*m - barh)/2,
                t,
                barh - 2*dot).Fill(warn);

    p.Rectangle(cx - dot/2,
                m + (tile - 2*m - dot)/2 + barh - dot,
                dot,
                dot).Fill(warn);
}

// ---------------- Ensure thumbs at current zoom ----------------
void IconGalleryCtrl::EnsureThumbs(IconGalleryItem& it) {
    int tile = zoom_steps[zoom_i];
    bool need_normal = it.thumb_normal.IsEmpty()
                    || it.thumb_normal.GetWidth() != tile
                    || it.thumb_normal.GetHeight() != tile;
    bool need_gray   = it.thumb_gray.IsEmpty()
                    || it.thumb_gray.GetWidth() != tile
                    || it.thumb_gray.GetHeight() != tile;
    if(!need_normal && !need_gray) return;

    if(!it.src.IsEmpty()) {
        Image scaled = Rescale(it.src, Size(tile, tile));
        if(need_normal) it.thumb_normal = scaled;

        if(need_gray) {
            ImageBuffer gb(tile, tile);
            const RGBA* sp = scaled.Begin();
            RGBA*       gp = gb.Begin();
            for(int i = 0; i < tile * tile; ++i, ++sp, ++gp) {
                int lum = (sp->r*30 + sp->g*59 + sp->b*11) / 100;
                gp->r = gp->g = gp->b = (byte)lum;
                gp->a = sp->a;
            }
            gb.End();
            it.thumb_gray = gb;
        }
        return;
    }

    // No src: draw status glyphs (or auto dummy)
    if(need_normal) {
        switch(it.status) {
        case ThumbStatus::Placeholder: it.thumb_normal = MakePlaceholderGlyph(tile, false); break;
        case ThumbStatus::Missing:     it.thumb_normal = MakeMissingGlyph(tile, false);     break;
        case ThumbStatus::Auto: {
            ImageBuffer ib(tile, tile);
            RGBA* p = ib.Begin();
            for(int i = 0; i < tile * tile; ++i, ++p) { p->r = p->g = p->b = 0; p->a = 255; }
            ib.End();
            BufferPainter bp(ib);
            Color a = Color(24,28,34), b = Color(18,22,27);
            int step = 8;
            for(int y = 0; y < tile; y += step)
                for(int x = 0; x < tile; x += step)
                    bp.Rectangle(x, y, step, step).Fill(((x + y) / step) % 2 ? a : b);
            int m = max(2, tile / 8);
            bp.Rectangle(m, m, tile - 2*m, tile - 2*m).Fill(it.seed);
            it.thumb_normal = ib;
            break;
        }}
    }

    if(need_gray) {
        switch(it.status) {
        case ThumbStatus::Placeholder: it.thumb_gray = MakePlaceholderGlyph(tile, true); break;
        case ThumbStatus::Missing:     it.thumb_gray = MakeMissingGlyph(tile, true);     break;
        case ThumbStatus::Auto: {
            ImageBuffer gb(tile, tile);
            RGBA* gp = gb.Begin();
            for(int i = 0; i < tile * tile; ++i, ++gp) { gp->r = gp->g = gp->b = 0; gp->a = 255; }
            gb.End();
            BufferPainter gpb(gb);
            Color a = Color(24,28,34), b = Color(18,22,27);
            int step = 8;
            for(int y = 0; y < tile; y += step)
                for(int x = 0; x < tile; x += step)
                    gpb.Rectangle(x, y, step, step).Fill(((x + y) / step) % 2 ? a : b);
            int m = max(2, tile / 8);
            int lum = (it.seed.GetR()*30 + it.seed.GetG()*59 + it.seed.GetB()*11)/100;
            Color gray(lum, lum, lum);
            gpb.Rectangle(m, m, tile - 2*m, tile - 2*m).Fill(gray);
            it.thumb_gray = gb;
            break;
        }}
    }
}

// ---------------- Paint & input ----------------
Color IconGalleryCtrl::AutoColorFromText(const String& s) const {
    unsigned acc = 0;
    for(byte ch : s) acc = acc * 131 + ch;
    double h  = (acc % 360) / 360.0;
    double sN = 0.45 + ((acc % 60) / 60.0) * 0.25;
    double vN = 0.55 + (((acc / 7) % 50) / 50.0) * 0.25;
    return HsvColorf(h, sN, vN);
}

void IconGalleryCtrl::StrokeRect(Draw& w, const Rect& r, int t, const Color& c) const {
    if(t <= 0) return;
    w.DrawRect(RectC(r.left, r.top, r.Width(), t), c);
    w.DrawRect(RectC(r.left, r.bottom - t, r.Width(), t), c);
    w.DrawRect(RectC(r.left, r.top, t, r.Height()), c);
    w.DrawRect(RectC(r.right - t, r.top, t, r.Height()), c);
}

void IconGalleryCtrl::Paint(Draw& w) {
    Size sz = GetSize();
    w.DrawRect(sz, SColorPaper());

    if(items.IsEmpty()) {
        w.DrawText(10, 10, "Gallery empty — use Save to add icons", StdFont(), SColorDisabled());
        return;
    }

    Rect vr(0, 0, sz.cx, sz.cy);
    int tile = zoom_steps[zoom_i];
    int boxH = tile + labelH + 2*pad;

    int firstRow = max(0, (scroll_y - pad) / (boxH + pad));
    int lastRow  = (scroll_y + sz.cy - pad) / (boxH + pad) + 1;

    for(int r = firstRow; r <= lastRow; ++r) {
        for(int c = 0; c < cols; ++c) {
            int i = r * cols + c;
            if(i >= items.GetCount()) break;

            Rect box = IndexRect(i);
            if(!box.Intersects(vr)) continue;

            auto& it = items[i];
            EnsureThumbs(it);

            // base panel
            w.DrawRect(box, Blend(SColorFace(), SColorPaper(), 200));

            // hover tint (subtle)
            if(i == hover_index && !it.selected) {
                Color tint = Blend(SColorHighlight(), SColorFace(), 220);
                w.DrawRect(box, tint);
            }

            if(show_filter_border && !it.filtered_out)
                StrokeRect(w, box, 1, SColorPaper());

            const bool want_gray = !saturation_on || it.filtered_out;
            const Image& thumb = want_gray && !it.thumb_gray.IsEmpty()
                               ? it.thumb_gray : it.thumb_normal;

            Size ts = thumb.GetSize();
            Point p = box.TopLeft() + Point((box.GetWidth() - ts.cx) / 2, pad);
            w.DrawImage(p.x, p.y, thumb);

            Rect lab = RectC(box.left, box.bottom - labelH - pad, box.GetWidth(), labelH + pad);
            w.DrawRect(lab, SColorLtFace());
            String caption = it.name;
            if(caption.GetLength() > 24) caption = caption.Left(21) + "...";
            const Color textc = (want_gray ? SColorDisabled() : SColorText());
            w.DrawText(lab.left + 6, lab.top + (labelH - StdFont().GetHeight()) / 2,
                       caption, StdFont(), textc);

            if(show_selection_border && it.selected)
                StrokeRect(w, box, 2, SColorHighlight());
        }
    }
}

void IconGalleryCtrl::LeftDown(Point p, dword flags) {
    SetFocus();
    bool ctrl  = (flags & K_CTRL) != 0;
    bool shift = (flags & K_SHIFT) != 0;

    for(int i = 0; i < items.GetCount(); ++i) if(IndexRect(i).Contains(p)) {
        if(shift && anchor_index >= 0) {
            SelectRange(anchor_index, i, ctrl); // ctrl keeps existing, otherwise replace
        } else {
            if(!ctrl) for(auto& it : items) it.selected = false;
            items[i].selected = ctrl ? !items[i].selected : true;
            anchor_index = i; // update anchor for future shift
        }
        Refresh();
        if(WhenSelection) WhenSelection();
        return;
    }
}

void IconGalleryCtrl::LeftDouble(Point p, dword) {
    for(int i = 0; i < items.GetCount(); ++i)
        if(IndexRect(i).Contains(p) && WhenActivate) { WhenActivate(items[i]); return; }
}

void IconGalleryCtrl::RightDown(Point p, dword) {
    ShowContextMenu(p);
}

void IconGalleryCtrl::MouseMove(Point p, dword) {
    int new_hover = -1;
    for(int i = 0; i < items.GetCount(); ++i)
        if(IndexRect(i).Contains(p)) { new_hover = i; break; }
    if(new_hover != hover_index) { hover_index = new_hover; Refresh(); }
}

bool IconGalleryCtrl::Key(dword key, int) {
    // Let ScrollBars handle page/home/end/arrows etc.
    if(sb.Key(key)) {
        scroll_y = sb.GetY();
        Refresh();
        return true;
    }

    if(key == K_ENTER) {
        Vector<int> sel = GetSelection();
        if(sel.GetCount() && WhenActivate) { WhenActivate(items[sel[0]]); return true; }
    }
    return false;
}

void IconGalleryCtrl::MouseWheel(Point, int zdelta, dword keyflags) {
    if(keyflags & K_CTRL) { SetZoomIndex(zoom_i + (zdelta > 0 ? +1 : -1)); return; }
    sb.WheelY(zdelta);
    scroll_y = sb.GetY();
    Refresh();
}

// ---------------- Context menu ----------------
void IconGalleryCtrl::ShowContextMenu(Point p) {
    MenuBar::Execute([&](Bar& bar){
        bar.Add("Select all",      [&]{ DoSelectAll();      });
        bar.Add("Invert selection",[&]{ DoInvertSelection();});
        bar.Add("Clear selection", [&]{ DoClearSelection(); });
        bar.Separator();
        bar.Add("Remove selected", [&]{ DoRemoveSelected(); });
        bar.Add("Remove all",      [&]{ DoRemoveAll();      });
    });
}

void IconGalleryCtrl::DoSelectAll() {
    for(auto& it : items) it.selected = true;
    Refresh(); if(WhenSelection) WhenSelection();
}
void IconGalleryCtrl::DoInvertSelection() {
    for(auto& it : items) it.selected = !it.selected;
    Refresh(); if(WhenSelection) WhenSelection();
}
void IconGalleryCtrl::DoClearSelection() {
    for(auto& it : items) it.selected = false;
    Refresh(); if(WhenSelection) WhenSelection();
}
void IconGalleryCtrl::DoRemoveSelected() {
    Vector<IconGalleryItem> keep;
    keep.Reserve(items.GetCount());
    for(auto& it : items) if(!it.selected) keep.Add(pick(it));
    items = pick(keep);
    anchor_index = hover_index = -1;
    Reflow(); Refresh(); if(WhenSelection) WhenSelection();
}
void IconGalleryCtrl::DoRemoveAll() {
    items.Clear();
    anchor_index = hover_index = -1;
    Reflow(); Refresh(); if(WhenSelection) WhenSelection();
}
