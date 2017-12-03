#ifndef _KUDD_BUS_REMINDER_SETTINGS_H_
#define _KUDD_BUS_REMINDER_SETTINGS_H_

#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace kudd {
    class BusDisplayOption {
    public:
        BusDisplayOption()
            : _bgColor(L"0x000000")
            , _textColor(L"0xffffff")
        {
        }

        BusDisplayOption(std::wstring&& bgColor, std::wstring&& textColor)
            : _bgColor(std::move(bgColor))
            , _textColor(std::move(textColor))
        {
        }

    public:
        const std::wstring& bgColor()   const { return _bgColor;   }
        const std::wstring& textColor() const { return _textColor; }

    private:
        std::wstring _bgColor;
        std::wstring _textColor;
    };

    ////////////////////////////////////////////////////////////////////////////////////

    class SubscribeInfo {
    public:
        enum Direction : uint8_t {
            NONE = 0,
            NORTH,
            WEST,
            EAST,
            SOUTH,
        };

    public:
        SubscribeInfo()
            : _direction(NONE)
        {
        }

    public:
        Direction direction()          const { return _direction; }
        const std::wstring& stopName() const { return _stopName;  }
        const std::wstring& stopId()   const { return _stopId;    }

        const auto& time()     const { return _time;     }
        const auto& busLines() const { return _busLines; }
        inline const auto& findDisplayOption(const std::wstring& busLine) const;

        void setDirection(Direction d)                  { _direction = d;       }
        void setStopName(const std::wstring& sn)        { _stopName = sn;       }
        void setStopId(const std::wstring& si)          { _stopId = si;         }
        void setTime(std::pair<uint16_t, uint16_t>&& t) { _time = std::move(t); }

        inline void addBusLine(const std::wstring& line, BusDisplayOption&& option);

    private:
        Direction _direction;
        std::wstring _stopName;
        std::wstring _stopId;
        std::pair<uint16_t, uint16_t> _time;
        std::map<std::wstring, BusDisplayOption> _busLines;     // key : busLine
    };

    inline const auto& SubscribeInfo::findDisplayOption(const std::wstring& busLine) const
    {
        static BusDisplayOption invalid;
        const auto where = _busLines.find(busLine);
        return where == _busLines.end() ? invalid : where->second;
    }

    inline void SubscribeInfo::addBusLine(const std::wstring& line, BusDisplayOption&& option)
    {
        _busLines[line] = std::move(option);
    }

    ////////////////////////////////////////////////////////////////////////////////////

    class Configuration {
    public:
        static Configuration& get();

    private:
        Configuration();

    public:
        bool loadFrom(const std::wstring& filename);
        bool saveTo(const std::wstring& filename) const;

    public:
        const auto& subscriptions() const { return _subscriptions; }
        inline const auto& findSubscribeInfo(const std::wstring& stopId);
        inline const auto& findDisplayOption(const std::wstring& stopId, const std::wstring& line);

    private:
        void makeDefault();

    private:
        std::map<std::wstring, SubscribeInfo> _subscriptions;       // key : stopId
    };

    inline const auto& Configuration::findSubscribeInfo(const std::wstring& stopId)
    {
        static SubscribeInfo invalid;
        const auto where = _subscriptions.find(stopId);
        return where == _subscriptions.end() ? invalid : where->second;
    }

    inline const auto& Configuration::findDisplayOption(const std::wstring& stopId, const std::wstring& line)
    {
        static BusDisplayOption invalid;
        const auto where = _subscriptions.find(stopId);
        return where == _subscriptions.end() ? invalid : where->second.findDisplayOption(line);
    }

    ////////////////////////////////////////////////////////////////////////

}

#endif // _KUDD_BUS_REMINDER_SETTINGS_H_

// 끝.
