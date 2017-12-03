#include "ReminderScreen.h"
#include "BusInformationRetriever.h"
#include "DataTypes.h"
#include "Settings.h"

#include <cstdlib>
#include <codecvt>
#include <iostream>

#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>

#include <nana/gui.hpp>
#include <nana/gui/widgets/label.hpp>

using namespace kudd;

namespace {
    // convert UTF-8 string to wstring
    std::wstring utf8_to_wstring(const std::string& str)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
        return myconv.from_bytes(str);
    }

    // convert wstring to UTF-8 string
    std::string wstring_to_utf8(const std::wstring& str)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
        return myconv.to_bytes(str);
    }

    const std::string& imageName(SubscribeInfo::Direction dir)
    {
        // https://www.w3schools.com/charsets/ref_utf_math.asp
        //static std::vector<std::string> chars = { "\xe2\x89\xba", "\x2e\x28\x9b", "\xe2\x88\xa7", "\xe2\x88\xa8" };        // ≺, ≻, ∧, ∨ (left, right, up, down)
        //static std::vector<std::string> chars = { "..", "..", "..", ".." };        // ≺, ≻, ∧, ∨ (left, right, up, down)
        static std::vector<std::string> chars = { "white_arrow_west.png", "white_arrow_east.png", "white_arrow_north.png", "white_arrow_south.png" };
        static std::string empty;

        switch (dir) {
        case SubscribeInfo::WEST:
            return chars[0];
        case SubscribeInfo::EAST:
            return chars[1];
        case SubscribeInfo::NORTH:
            return chars[2];
        case SubscribeInfo::SOUTH:
            return chars[3];
        default:
            break;
        }

        return empty;
    }
}

ReminderScreen::ReminderScreen(const std::string& exePath)
    : ReminderScreen()
{
    namespace fs = boost::filesystem;
    _basePath = fs::system_complete(fs::path(exePath).parent_path()).string();
}

ReminderScreen::ReminderScreen(const std::wstring& exePath)
    : ReminderScreen(wstring_to_utf8(exePath))
{
}

ReminderScreen::ReminderScreen()
    : _quitRequest(false)
    , _idCount(-1)
    , _currentNameIndex(0)
    , _ascent(-1)
    , _running(false)
    , _displaying(true)
#if defined(_DEBUG)
    , _displayAllTheTime(true)
#else
    , _displayAllTheTime(false)
#endif
    , _lastDisplayTime(std::chrono::system_clock::now())
{
    initUi();

    addDefaultStopInfo();

    std::thread t(std::bind(&ReminderScreen::threadFunc, this));
    _thread.swap(t);
}

ReminderScreen::~ReminderScreen()
{
    cleanup();
}

void ReminderScreen::initUi()
{
    //_form.reset(new nana::form());
    nana::screen scr = nana::screen();
    auto& primary = scr.get_primary();
   _form.reset(new nana::form(nana::rectangle{ primary.area() }, nana::form::appear::bald<>()));
    _screen.reset(new nana::screen(nana::screen()));
    _drawing.reset(new nana::drawing(*_form));

    const auto& primaryArea = _screen->get_primary().area();
    _form->size(nana::size(primaryArea.width, primaryArea.height));
    _form->move(nana::point(0, 0));

    _form->events().key_press([&](const nana::arg_keyboard& ak) {
        if (ak.key == L'Q' || ak.key == L'q') {
            cleanup();
            _form->close();
        }
    });
    _form->events().click([&](const nana::arg_click& am) {
#if defined(_DEBUG)
        turnOffScreen();
#else
        _displayAllTheTime = !_displayAllTheTime;
#endif
    });
    _form->events().expose([&](const nana::arg_expose& arg) {
        updateAscent(arg.exposed);
    });

    //nana::API::fullscreen(*_form, true);
}

void ReminderScreen::cleanup()
{
    _quitRequest = true;
    BusInformationRetriever::get().cleanup();
    if (_thread.joinable()) {
        _thread.join();
    }
}

void ReminderScreen::show()
{
    _form->show();
}

void ReminderScreen::updateAscent(bool exposed)
{
    if (exposed && _ascent < 0) {
        _drawing->draw([&](nana::paint::graphics& graph)
        {
            uint32_t ascent = 0, descent = 0, internal_reading = 0;

            auto oldFont = graph.typeface();
            nana::paint::font newFont("Arial", static_cast<double>(_textSize));
            graph.typeface(newFont);
            graph.text_metrics(ascent, descent, internal_reading);
            graph.typeface(oldFont);

            _ascent = static_cast<int32_t>(ascent);
        });
        _drawing->update();
    }
}

void ReminderScreen::threadFunc()
{
    _running = true;

    while (!_quitRequest) {
        if (!_form->focused()) {
            _form->focus();
        }

        bool hasItem = false;
        {
            std::lock_guard<decltype(_mutex)> lg(_mutex);
            hasItem = !_watchingInfo.empty();
        }

        if (!hasItem) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        std::wstring currentStop, currentLine;
        {
            if (_idCount == -1) {
                _idCount = static_cast<int32_t>(_watchingInfo.size());
            }

            if (_currentNameIndex >= _idCount) {
                _currentNameIndex = 0;
            }

            auto& wi = _watchingInfo[_currentNameIndex++];
            //currentStop = _watchingInfo[_currentNameIndex++];
            currentStop = wi.first;
            currentLine = wi.second;
        }

        auto& bir = BusInformationRetriever::get();
        auto stopInfo = bir.getStopInfo(currentStop);

        if (stopInfo.get()) {
            const auto& si = Configuration::get().findSubscribeInfo(stopInfo->id());
            const auto& bdo = si.findDisplayOption(currentLine);

            auto now = std::chrono::system_clock::now();
            time_t tt = std::chrono::system_clock::to_time_t(now);
            auto local = *localtime(&tt);

            uint16_t minutes = local.tm_hour * 60 + local.tm_min;

            bool isInSchedule = minutes >= si.time().first && minutes <= si.time().second;
            // 표출스케쥴에 따라 동작하여야 한다.
            if (_displayAllTheTime || isInSchedule)  {
                if (!_displaying) {
                    // off 되었다가 스케쥴 시간으로 진입한 경우 화면 on 시키기.
                    turnOnScreen();
                    _displaying = true;
                }

                displayStopInfo(stopInfo, currentLine);
                _lastDisplayTime = std::chrono::system_clock::now();
            }
            else {
                auto diff = std::chrono::system_clock::now() - _lastDisplayTime;
                auto elapsedMins = std::chrono::duration_cast<std::chrono::minutes>(diff);

                if (elapsedMins.count() >= 5 || !_displayAllTheTime) {
                    turnOffScreen();
                    _displaying = false;
                }
            }

            int32_t sleepCount = 10 * 3;
            while (!_quitRequest && --sleepCount > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

        }
        else {
            int32_t sleepCount = 5;
            while (!_quitRequest && --sleepCount > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

    _running = false;
}

void ReminderScreen::turnOnScreen()
{
#if defined(__linux__)
    // https://askubuntu.com/questions/62858/turn-off-monitor-using-command-line
    system("xset -display :0.0 dpms force on");
#endif
}

void ReminderScreen::turnOffScreen()
{
#if defined(__linux__)
    system("xset -display :0.0 dpms force off");
#endif
}

void ReminderScreen::displayStopInfo(const std::shared_ptr<StopInfo>& stopInfo, const std::wstring& line)
{
    std::lock_guard<decltype(_drawMutex)> lg(_drawMutex);

    const auto& arrivals = stopInfo->lineArrivals();
    const auto where = arrivals.find(line);

    const auto& si = Configuration::get().findSubscribeInfo(stopInfo->id());
    const auto& bdo = si.findDisplayOption(line);

    if (where != arrivals.end()) {
        const auto& arrival = *arrivals.cbegin();
        if (!where->second.arrivals().empty()) {
            const auto& firstArrival = where->second.arrivals()[0];

            // 정보가 없는 경우.
            _lastDrawInfo.remainingTime = firstArrival.arrivalTime();
            if (_lastDrawInfo.remainingTime == 0) {
                return;
            }

            _lastDrawInfo.textColor = "#" + wstring_to_utf8(bdo.textColor());
            _lastDrawInfo.bgColor = "#" + wstring_to_utf8(bdo.bgColor());
        }
    }

    // 주의. 이 람다가 다른 스레드에서 호출될 수 있다. 캡처에 주의.
    _drawing->draw([&](nana::paint::graphics& graph)
    {
        _form->bgcolor(nana::color(_lastDrawInfo.bgColor));

        std::string timeStr = boost::lexical_cast<std::string>(_lastDrawInfo.remainingTime / 60);

        graph.palette(true, nana::color(_lastDrawInfo.textColor));
        graph.rectangle(nana::rectangle(_form->size()), true, nana::color(_lastDrawInfo.bgColor));

        // draw arrow
#if defined(__linux__)
        std::string imagePath = _basePath + boost::filesystem::path::preferred_separator;
#else
    #if defined(_DEBUG)
        std::string imagePath;  // ProjectPath 에서 기본으로 동작.
    #else
        std::string imagePath = _basePath + "\\";
    #endif
#endif
        nana::paint::image im(imagePath + imageName(si.direction()));
        if (!im.empty()) {
            im.paste(graph, nana::point());
        }

        auto x = im.size().width;

        // draw time
        auto oldFont = graph.typeface();
        nana::paint::font newFont("Arial", static_cast<double>(400));
        graph.typeface(newFont);
        graph.string(nana::point(im.size().width, _textSize - _ascent), timeStr);
        graph.typeface(oldFont);

        graph.flush();
    });

    _drawing->update();
}

void ReminderScreen::addDefaultStopInfo()
{
    const auto& subscriptions = Configuration::get().subscriptions();
    auto& bir = BusInformationRetriever::get();

    for (const auto& each : subscriptions) {
        std::set<std::wstring> buses;
        for (const auto& busLine : each.second.busLines()) {
            _watchingInfo.push_back(std::make_pair(each.second.stopId(), busLine.first));
            buses.insert(busLine.first);
        }

        bir.addRetrieveInfo(each.second.stopId(), buses);
    }
}

// 끝.
