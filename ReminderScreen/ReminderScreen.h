#ifndef _KUDD_REMINDER_SCREEN_H_
#define _KUDD_REMINDER_SCREEN_H_

#include <thread>
#include <mutex>
#include <vector>
#include <memory>
#include <atomic>
#include <chrono>

namespace nana {
    class form;
    class screen;
    class drawing;
}

namespace kudd {
    class BusInformationRetriever;
    class StopInfo;

    struct LastDrawInfo {
        LastDrawInfo()
            : remainingTime(0)
        {
        }

        std::string textColor;
        std::string bgColor;
        int32_t remainingTime;
    };

    class ReminderScreen {
    public:
        ReminderScreen(const std::string& exePath);
        ReminderScreen(const std::wstring& exePath);
        ~ReminderScreen();

    private:
        ReminderScreen();

    public:
        void show();

    private:
        void initUi();
        void cleanup();
        void threadFunc();
        void displayStopInfo(const std::shared_ptr<StopInfo>& stopInfo, const std::wstring& line);
        void addDefaultStopInfo();

        void turnOnScreen();
        void turnOffScreen();

        void updateAscent(bool exposed);

    private:
        std::unique_ptr<nana::form> _form;
        std::unique_ptr<nana::screen> _screen;
        std::unique_ptr<nana::drawing> _drawing;

        std::atomic_bool _quitRequest;

    private:
        std::mutex _mutex;
        std::mutex _drawMutex;
        std::thread _thread;
        std::vector<std::pair<std::wstring, std::wstring> > _watchingInfo;
        int32_t _idCount;
        int32_t _currentNameIndex;

        const int32_t _textSize = 400;
        int32_t _ascent;

        LastDrawInfo _lastDrawInfo;

        std::string _basePath;

        bool _running;
        bool _displaying;
        bool _displayAllTheTime;
        std::chrono::time_point<std::chrono::system_clock> _lastDisplayTime;
    };
}

#endif // _KUDD_REMINDER_SCREEN_H_

// 끝.

