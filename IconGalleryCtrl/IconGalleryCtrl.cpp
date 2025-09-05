#include "IconGalleryCtrl.h"

static inline int ClampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

/*================ Construction ================*/
IconGalleryCtrl::IconGalleryCtrl() {
    BackPaint();
    AddFrame(sb);
    sb.AutoHide(true).NoBox();            // clean look; shows only when needed
    sb.SetLine(20, 80);                   // wheel/arrow step (x,y). y is roughly one label line
    sb.WhenScroll = [=] { Refresh(); };   // repaint on scroll
}

/*================ Item add ====================*/
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

/*============= Attach / clear real image =============*/
bool IconGalleryCtrl::SetThumbFromFile(int index, const String& filepath) {
    if(index < 0 || index >= items.GetCount()) return false;
    Image m = StreamRaster::LoadFileAny(filepath);
    if(IsNull(m)) {
        SetThumbStatus(index, ThumbStatus::Missing);
        return false;
    }
    SetThumbImage(index, m);
    return true;
}

void IconGalleryCtrl::SetThumbImage(int index, const Image& img) {
    if(index < 0 || index >= items.GetCount()) return;
    auto& it = items[index];
    it.src = img;                       // keep full-res copy
    it.status = ThumbStatus::Auto;      // src will be used at EnsureThumbs
    it.thumb_normal = Image();          // invalidate caches
    it.thumb_gray   = Image();
    Refresh();
}

void IconGalleryCtrl::ClearThumbImage(int index) {
    if(index < 0 || index >= items.GetCount()) return;
    auto& it = items[index];
    it.src = Image();                   // drop source
    it.thumb_normal = Image();
    it.thumb_gray   = Image();
    Refresh();
}

/*================ Status toggle ====================*/
void IconGalleryCtrl::SetThumbStatus(int index, ThumbStatus s) {
    if(index < 0 || index >= items.GetCount()) return;
    auto& it = items[index];
    if(it.status == s) return;
    it.status = s;
    it.thumb_normal = Image();
    it.thumb_gray   = Image();
    Refresh();
}

/*================ Selection / filter ================*/
Vector<int> IconGalleryCtrl::GetSelection() const {
    Vector<int> out;
    for(int i = 0; i < items.GetCount(); ++i)
        if(items[i].selected) out.Add(i);
    return out;
}

void IconGalleryCtrl::SetFiltered(int index, bool filtered_out) {
    if(index < 0 || index >= items.GetCount()) return;
    items[index].filtered_out = filtered_out;
    Refresh();
}

void IconGalleryCtrl::ClearFilterFlags() {
    for(auto& it : items) it.filtered_out = false;
    Refresh();
}

/*================ Zoom ====================*/
void IconGalleryCtrl::SetZoomIndex(int zi) {
    zi = ClampInt(zi, 0, zoom_steps.GetCount() - 1);
    if(zi == zoom_i) return;
    zoom_i = zi;
    if(WhenZoom) WhenZoom(zoom_i);
    for(auto& it : items) { it.thumb_normal = Image(); it.thumb_gray = Image(); }
    Reflow(); Refresh();
}

/*================ Layout / flow ====================*/
void IconGalleryCtrl::Reflow() {
    Size sz  = GetSize();
    int tile = zoom_steps[zoom_i];
    int boxW = tile + 2*pad;
    int boxH = tile + labelH + 2*pad;

    cols = max(1, (sz.cx - pad) / (boxW + pad));

    int rows = items.GetCount() ? ((items.GetCount() - 1) / cols + 1) : 0;
    content_h = pad + rows * (boxH + pad);

    // Tell the scrollbars what the viewport and content sizes are.
    sb.SetPage(sz);                       // visible area
    sb.SetTotal(Size(sz.cx, content_h));  // only vertical scrolling for now
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

Rect IconGalleryCtrl::IndexRectView(int i) const {
    Rect raw = IndexRectNoScroll(i);
    return raw.Offseted(0, -sb.GetY());  // bring into viewport space
}

/*================ Glyph builders ====================*/
Image IconGalleryCtrl::MakePlaceholderGlyph(int tile, bool gray) {
    ImageBuffer ib(tile, tile);
    RGBA* p = ib.Begin();
    for(int i = 0; i < tile * tile; ++i, ++p) { p->r = p->g = p->b = 0; p->a = 255; }
    ib.End();
    BufferPainter bp(ib);
    // checker
    Color a = Color(24,28,34), b = Color(18,22,27);
    int step = 8;
    for(int y = 0; y < tile; y += step)
        for(int x = 0; x < tile; x += step)
            bp.Rectangle(x, y, step, step).Fill(((x + y) / step) % 2 ? a : b);
    // dashed box + plus
    IconGalleryCtrl().DrawPlaceholderGlyph(bp, tile, gray);
    return ib;
}

Image IconGalleryCtrl::MakeMissingGlyph(int tile, bool gray) {
    ImageBuffer ib(tile, tile);
    RGBA* p = ib.Begin();
    for(int i = 0; i < tile * tile; ++i, ++p) { p->r = p->g = p->b = 0; p->a = 255; }
    ib.End();
    BufferPainter bp(ib);
    // checker
    Color a = Color(24,28,34), b = Color(18,22,27);
    int step = 8;
    for(int y = 0; y < tile; y += step)
        for(int x = 0; x < tile; x += step)
            bp.Rectangle(x, y, step, step).Fill(((x + y) / step) % 2 ? a : b);
    // warning glyph
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

/*================ Simple primitives ====================*/
void IconGalleryCtrl::DrawPlaceholderGlyph(BufferPainter& p, int tile, bool gray) const {
    Color edge = gray ? SColorDisabled() : SColorText();
    int   m    = max(2, tile / 10);
    int   t    = max(1, tile / 16);
    int dash = max(2, tile / 12);
    int gap  = dash;

    for(int x = m; x < tile - m; x += dash + gap) {
        int w = min(dash, tile - m - x);
        p.Rectangle(x, m, w, t).Fill(edge);                       // top
        p.Rectangle(x, tile - m - t, w, t).Fill(edge);            // bottom
    }
    for(int y = m; y < tile - m; y += dash + gap) {
        int h = min(dash, tile - m - y);
        p.Rectangle(m, y, t, h).Fill(edge);                       // left
        p.Rectangle(tile - m - t, y, t, h).Fill(edge);            // right
    }
    int cx = tile / 2, cy = tile / 2, arm = (tile - 2*m) / 3;
    p.Rectangle(cx - t/2, cy - arm, t, 2*arm).Fill(edge);
    p.Rectangle(cx - arm, cy - t/2, 2*arm, t).Fill(edge);
}

void IconGalleryCtrl::DrawMissingGlyph(BufferPainter& p, int tile, bool gray) const {
    const Color warn = gray ? SColorDisabled() : Color(200, 60, 60);
    const int   m    = max(2, tile / 10);
    const int   t    = max(2, tile / 14);

    // frame (four sides)
    p.Rectangle(m, m,                tile - 2*m, t          ).Fill(warn);
    p.Rectangle(m, tile - m - t,     tile - 2*m, t          ).Fill(warn);
    p.Rectangle(m, m,                t,          tile - 2*m ).Fill(warn);
    p.Rectangle(tile - m - t, m,     t,          tile - 2*m ).Fill(warn);

    // exclamation mark
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

/*================ Ensure thumbs at current zoom ====================*/
void IconGalleryCtrl::EnsureThumbs(IconGalleryItem& it) {
    const int tile = zoom_steps[zoom_i];
    const bool need_normal = it.thumb_normal.IsEmpty()
                          || it.thumb_normal.GetWidth()  != tile
                          || it.thumb_normal.GetHeight() != tile;
    const bool need_gray   = it.thumb_gray.IsEmpty()
                          || it.thumb_gray.GetWidth()  != tile
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

/*================ Paint & input ====================*/
Color IconGalleryCtrl::AutoColorFromText(const String& s) const {
    unsigned acc = 0;
    for(byte ch : s) acc = acc * 131 + ch;
    double h  = (acc % 360) / 360.0;
    double sN = 0.45 + ((acc % 60) / 60.0) * 0.25;
    double vN = 0.55 + (((acc / 7) % 50) / 50.0) * 0.25;
    return HsvColorf(h, sN, vN);
}

void IconGalleryCtrl::StrokeRect(Draw& w, const Rect& r, int t, const Color& c) {
    if(t <= 0) return;
    w.DrawRect(RectC(r.left, r.top, r.Width(), t), c);
    w.DrawRect(RectC(r.left, r.bottom - t, r.Width(), t), c);
    w.DrawRect(RectC(r.left, r.top, t, r.Height()), c);
    w.DrawRect(RectC(r.right - t, r.top, t, r.Height()), c);
}

void IconGalleryCtrl::Layout() {
    Reflow();
    Refresh();
}

void IconGalleryCtrl::Paint(Draw& w) {
    Size sz = GetSize();
    w.DrawRect(sz, SColorPaper());

    if(items.IsEmpty()) {
        w.DrawText(10, 10, "Gallery empty — use Save to add icons", StdFont(), SColorDisabled());
        return;
    }

    const int tile = zoom_steps[zoom_i];
    const int boxH = tile + labelH + 2*pad;

    const int view_top    = sb.GetY();
    const int view_bottom = view_top + sz.cy;

    const int firstRow = max(0, (view_top   - pad) / (boxH + pad));
    const int lastRow  =       (view_bottom - pad) / (boxH + pad) + 1;

    for(int r = firstRow; r <= lastRow; ++r) {
        for(int c = 0; c < cols; ++c) {
            const int i = r * cols + c;
            if(i >= items.GetCount()) break;

            const Rect box = IndexRectView(i);
            const Rect vr(0, 0, sz.cx, sz.cy);
            if(!box.Intersects(vr)) continue;

            auto& it = items[i];
            EnsureThumbs(it);

            // background panel
            w.DrawRect(box, Blend(SColorFace(), SColorPaper(), 200));

            // thin "filtered-in" halo (under selection)
            if(show_filter_border && !it.filtered_out)
                StrokeRect(w, box, 1, SColorPaper());

            const bool want_gray = !saturation_on || it.filtered_out;
            const Image& thumb = want_gray && !it.thumb_gray.IsEmpty()
                               ? it.thumb_gray : it.thumb_normal;

            // image centered
            Size ts = thumb.GetSize();
            Point p = box.TopLeft() + Point((box.GetWidth() - ts.cx) / 2, pad);
            w.DrawImage(p.x, p.y, thumb);

            // label area
            Rect lab = RectC(box.left, box.bottom - labelH - pad, box.GetWidth(), labelH + pad);
            w.DrawRect(lab, SColorLtFace());
            String caption = it.name;
            if(caption.GetLength() > 24) caption = caption.Left(21) + "...";
            const Color textc = (want_gray ? SColorDisabled() : SColorText());
            w.DrawText(lab.left + 6, lab.top + (labelH - StdFont().GetHeight()) / 2,
                       caption, StdFont(), textc);

            // selection ring
            if(show_selection_border && it.selected)
                StrokeRect(w, box, 2, SColorHighlight());
        }
    }
}

void IconGalleryCtrl::LeftDown(Point p, dword flags) {
    for(int i = 0; i < items.GetCount(); ++i) if(IndexRectView(i).Contains(p)) {
        bool add = (flags & K_CTRL) || (flags & K_SHIFT);
        if(!add) for(auto& it : items) it.selected = false;
        items[i].selected = add ? !items[i].selected : true;

        // ensure the clicked tile is visible
        Rect raw = IndexRectNoScroll(i);
        const int tile = zoom_steps[zoom_i];
        const int boxH = tile + labelH + 2*pad;
        sb.ScrollIntoY(raw.top, boxH);

        Refresh();
        if(WhenSelection) WhenSelection();
        return;
    }
}

void IconGalleryCtrl::LeftDouble(Point p, dword) {
    for(int i = 0; i < items.GetCount(); ++i)
        if(IndexRectView(i).Contains(p) && WhenActivate) { WhenActivate(items[i]); return; }
}

bool IconGalleryCtrl::Key(dword key, int) {
    // Let ScrollBars process navigation keys
    if(sb.Key(key)) { Refresh(); return true; }

    if(key == K_ENTER) {
        Vector<int> sel = GetSelection();
        if(sel.GetCount() && WhenActivate) { WhenActivate(items[sel[0]]); return true; }
    }
    return false;
}

void IconGalleryCtrl::MouseWheel(Point, int zdelta, dword keyflags) {
    if(keyflags & K_CTRL) {
        SetZoomIndex(zoom_i + (zdelta > 0 ? +1 : -1));
        return;
    }
    sb.WheelY(zdelta); // will trigger WhenScroll => Refresh()
}
