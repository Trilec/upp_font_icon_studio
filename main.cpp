#include <CtrlLib/CtrlLib.h>
using namespace Upp;

#include <IconGalleryCtrl/IconGalleryCtrl.h>   // package header

struct TopBar : public ParentCtrl {
    Label    lbl;
    DropList output;
    Button   exportBtn, saveBtn;

    TopBar() {
        BackPaint();

        Add(lbl.LeftPos(8, 80).VCenterPos(24));
        lbl.SetText("Output");

        Add(output.LeftPos(92, 100).VCenterPos(24));
        for(int s : {16,32,64,128,256}) output.Add(s);
        output.SetData(64);

        Add(exportBtn.LeftPos(200, 110).VCenterPos(24));
        exportBtn.SetLabel("Export PNG");

        Add(saveBtn.LeftPos(316, 90).VCenterPos(24));
        saveBtn.SetLabel("Save");
    }

    void Paint(Draw& w) override {
        Size sz = GetSize();
        w.DrawRect(sz, SColorFace());
        w.DrawLine(0, sz.cy - 1, sz.cx, sz.cy - 1, 1, SColorShadow()); // bottom hairline
    }
};

struct Pane : public ParentCtrl {
    String title;
    Pane(const String& t="") : title(t) { BackPaint(); }
    void Paint(Draw& w) override {
        Size sz = GetSize();
        w.DrawRect(sz, SColorPaper());
        // header strip
        w.DrawRect(RectC(0, 0, sz.cx, 24), Blend(SColorFace(), SColorPaper(), 180));
        w.DrawText(8, 5, title, StdFont().Bold(), SColorText());
        w.DrawLine(0, 24, sz.cx, 24, 1, SColorShadow());
    }
};

class MainWin : public TopWindow {
    TopBar         topbar;
    Pane           left   {"Preview Sizes"};
    Pane           center {"Stage"};
    Pane           right  {"Layer Tabs"};
    IconGalleryCtrl gallery;
public:
    MainWin() {
        Title("Font Icon Studio Pro — U++ Scaffold");
        Sizeable().Zoomable();

        // Normal child controls; positioned in Layout()
        Add(topbar);
        Add(left);
        Add(center);
        Add(right);
        Add(gallery);

        // Seed gallery with dummies
        for(int i = 0; i < 500; ++i)
            gallery.AddDummy(Format("Icon %d", i));

        // Simple actions
        topbar.exportBtn.WhenAction = [] { PromptOK("Export PNG (stub)"); };
        topbar.saveBtn.WhenAction   = [] { PromptOK("Save (stub)"); };
    }

    void Layout() override {
        // Grid like the HTML mockup:
        // columns: 320 | 1fr | 380 ; rows: topbar (32) | middle | gallery (160)
        Size ws = GetSize();
        int topH    = 32;
        int bottomH = 160;
        int leftW   = 320;
        int rightW  = 380;

        // top bar
        topbar.SetRect(0, 0, ws.cx, topH);

        // bottom gallery
        gallery.SetRect(0, ws.cy - bottomH, ws.cx, bottomH);

        // middle band
        int midY = topH;
        int midH = ws.cy - topH - bottomH;

        // left / right / center
        left.SetRect(0, midY, leftW, midH);
        right.SetRect(ws.cx - rightW, midY, rightW, midH);
        center.SetRect(leftW, midY, ws.cx - leftW - rightW, midH);
    }
};

GUI_APP_MAIN {
    SetLanguage(SetLNGCharset(LNGFromText("en-us"), CHARSET_UTF8));
    MainWin().Run();
}
