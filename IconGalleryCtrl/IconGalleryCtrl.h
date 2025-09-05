#pragma once
#include <CtrlLib/CtrlLib.h>
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
    bool  SetThumbFromFile(int index, const String& filepath);
    void  SetThumbImage(int index, const Image& img);
    void  ClearThumbImage(int index);

    // Status
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
    Vector<IconGalleryItem> items;

    // Layout / zoom
    Vector<int> zoom_steps {32, 48, 64, 80, 96, 112, 128};
    int         zoom_i = 2; // 64
    int         cols   = 1;
    int         pad    = 10;
    int         labelH = 16;

    // Manual scrolling state (vertical)
    int         scroll_y  = 0;
    int         content_h = 0;
    ScrollBars  sb;                // <— U++ ScrollBars frame

    // Selection helpers
    int         anchor_index = -1; // last “caret” for shift-range
    int         hover_index  = -1; // current hover tile or -1

    // Visual toggles
    bool        show_selection_border = true;
    bool        show_filter_border    = true;
    bool        saturation_on         = true;

    // Helpers
    void   Reflow();
    Rect   IndexRectNoScroll(int i) const;
    Rect   IndexRect(int i) const;
    void   EnsureThumbs(IconGalleryItem& it);
    Color  AutoColorFromText(const String& s) const;

    // Drawing helpers
    void   StrokeRect(Draw& w, const Rect& r, int t, const Color& c) const;
    void   DrawPlaceholderGlyph(BufferPainter& p, int tile, bool gray) const;
    void   DrawMissingGlyph(BufferPainter& p, int tile, bool gray) const;

    static Image MakePlaceholderGlyph(int tile, bool gray);
    static Image MakeMissingGlyph(int tile, bool gray);

    // Selection helpers
    void   SelectRange(int a, int b, bool additive);

    // Context menu
    void   ShowContextMenu(Point p);
    void   DoSelectAll();
    void   DoInvertSelection();
    void   DoClearSelection();
    void   DoRemoveSelected();
    void   DoRemoveAll();

    // Ctrl overrides
    void   Layout() override { Reflow(); Refresh(); }
    void   Paint(Draw& w) override;
    void   LeftDown(Point p, dword flags) override;
    void   LeftDouble(Point p, dword flags) override;
    void   RightDown(Point p, dword flags) override;
    void   MouseMove(Point p, dword flags) override;
    bool   Key(dword key, int) override;
    void   MouseWheel(Point p, int zdelta, dword keyflags) override;
};
