#if defined(_WIN32)
#include <windows.h>
#endif
#include <nana/gui.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/paint/image.hpp>

#include "ReminderScreen.h"

#if defined(_WIN32)
int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow)
#else
int main(int argc, char** argv)
#endif
{
    using namespace nana;

#if defined(__linux__)
    kudd::ReminderScreen screen(argv[0]);
#else
    wchar_t buffer[1024] = { 0, };
    GetModuleFileName(NULL, buffer, 1024);
    kudd::ReminderScreen screen(buffer);
#endif

    screen.show();

#if 0
    auto&& theScreen = screen();
    const auto& primaryArea = theScreen.get_primary().area();

    //Define a form.
    form fm;
    fm.size(size(800, 480));
    //form fm(rectangle{ screen().desktop_size() }, form::appear::bald<>());

    //Define a label and display a text.
    label lab{ fm, "<bold color=0x0000ff size=1000 font=\"Consolas\">11</>" };
    lab.format(true);

    //Define a button and answer the click event.
    button btn{ fm, "Quit" };
    btn.events().click([&fm, &lab] {
        //API::fullscreen(fm, true);
        //fm.size(nana::size(400, 600));
        //fm.close();
        lab.caption("Changed Label!!");
    });


#if 0
    paint::image im("test.jpg");
    drawing d(fm);

    d.draw([&im](paint::graphics& g) {
        //im.paste(g, point());
        //g.string(point(), L"7");
    });
#endif

    //Layout management
    //fm.div("vert <<><weight=80% text><>><weight=24<><button><>>");
    //fm.div("vert <weight=80% text><weight=24 button>");
    //fm.div("<><text><>");
    //fm.div("<<><vert weight=98% text><>>");
    //fm.div("vert <weight=98% text>");
    //fm["text"] << lab;
    //fm["button"] << btn;

    auto formSize = fm.size();

    lab.create(fm, true);
    lab.format(true);
    lab.caption("<bold color=0x0000ff size=550 font=\"Consolas\">8</>");
    lab.show();
    lab.move(rectangle(0, -200, formSize.width * 0.8, formSize.height));

    fm.collocate();

    lab.events().click([&lab] {
        // TODO: 편집 모드로 진입.
        lab.caption("<bold color=0x0000ff size=550 font=\"Consolas\">11</>");
    });

    //API::fullscreen(fm, true);

    //Show the form
    //fm.size(size(primaryArea.width, primaryArea.height));
    fm.show();
    auto s = lab.size();
#endif

    //Start to event loop process, it blocks until the form is closed.
    exec();
}

// 끝.
