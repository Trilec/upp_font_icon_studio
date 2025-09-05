#pragma once
#include <CtrlLib/CtrlLib.h>
#include <Painter/Painter.h> // BufferPainter

using namespace Upp;

// ---------- Data ----------
enum class ThumbStatus {
    Auto,        // generated dummy (checker + tinted block)
    Placeholder, // dashed box + plus
    Missing      // warning exclamation mark
};

struct IconGalleryItem : Moveable<IconGalleryItem> {
    String name;

    // Source image (optional): if non-empty, we scale it at current zoom
    Image  src;

    // Cached thumbs at current zoom
    Image  thumb_normal;
    Image  thumb_gray;

    Color  seed;            // deterministic tint from name
    bool   selected     = false;
    bool   filtered_out = false;
    ThumbStatus status   = ThumbStatus::Auto;
};

// ---------- Control ----------
class IconGalleryCtrl : public Ctrl {
public:
    Event<const IconGalleryItem&> WhenActivate;
    Event<>                       WhenSelection;
    Event<int>                    WhenZoom;

    IconGalleryCtrl();

    // Items
    int   Add(const String& name, const Image& opt_img = Image(), Color tint = Null);
    void  AddDummy(const String& name);

    // Attach / clear real image
    bool  SetThumbFromFile(int index, const String& filepath); // returns true if loaded
    void  SetThumbImage(int index, const Image& img);          // set from RAM
    void  ClearThumbImage(int index);                          // back to status glyphs

    // Quick status switches
    void  SetThumbStatus(int index, ThumbStatus s);

    // Selection
    Vector<int> GetSelection() const;

    // Filtering
    void  SetFiltered(int index, bool filtered_out);
    void  ClearFilterFlags();

    // Zoom
    void  SetZoomIndex(int zi);
    int   GetZoomIndex() const { return zoom_i; }

    // Visual toggles
    void  SetShowSelectionBorders(bool b) { show_selection_border = b; Refresh(); }
    void  SetShowFilterBorders(bool b)    { show_filter_border    = b; Refresh(); }
    void  SetSaturationOn(bool b)         { saturation_on         = b; Refresh(); }
    bool  GetShowSelectionBorders() const { return show_selection_border; }
    bool  GetShowFilterBorders() const    { return show_filter_border; }
    bool  GetSaturationOn() const         { return saturation_on; }

    // Reusable inline glyphs
    static const Image& PlaceholderGlyph(int tile = 32);
    static const Image& MissingGlyph(int tile = 32);

private:
    // Scroll frame
    ScrollBars sb; // AddFrame(sb); we only use vertical, AutoHide

    Vector<IconGalleryItem> items;

    // Layout / zoom
    Vector<int> zoom_steps {32, 48, 64, 80, 96, 112, 128};
    int         zoom_i = 2; // 64
    int         cols   = 1;
    int         pad    = 10;
    int         labelH = 16;
    int         content_h = 0;

    // Visual toggles
    bool        show_selection_border = true;
    bool        show_filter_border    = true;
    bool        saturation_on         = true;

    // Helpers
    void   Reflow();
    Rect   IndexRectNoScroll(int i) const;
    Rect   IndexRectView(int i) const; // position in viewport (y offset by sb)
    void   EnsureThumbs(IconGalleryItem& it);
    Color  AutoColorFromText(const String& s) const;

    // Drawing helpers
    static void StrokeRect(Draw& w, const Rect& r, int t, const Color& c);
    void   DrawPlaceholderGlyph(BufferPainter& p, int tile, bool gray) const;
    void   DrawMissingGlyph    (BufferPainter& p, int tile, bool gray) const;

    static Image MakePlaceholderGlyph(int tile, bool gray);
    static Image MakeMissingGlyph    (int tile, bool gray);

    // Ctrl overrides
    void   Layout() override;
    void   Paint(Draw& w) override;
    void   LeftDown(Point p, dword flags) override;
    void   LeftDouble(Point p, dword flags) override;
    bool   Key(dword key, int) override;
    void   MouseWheel(Point p, int zdelta, dword keyflags) override;
};
