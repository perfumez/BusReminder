#ifndef _KUDD_BUS_REMINDER_DATA_TYPES_H_
#define _KUDD_BUS_REMINDER_DATA_TYPES_H_

#include <string>
#include <cstdint>
#include <vector>
#include <map>
#include <chrono>

namespace kudd {
    class BusArrivalInfo {
    public:
        BusArrivalInfo()
            : _arrivalTime(-1)
            , _busStopCount(-1)
            , _isLast(false)
        {}

    public:
        const auto& vehicleType()   const { return _vehicleType;   }
        const auto& vehicleState()  const { return _vehicleState;  }
        const auto& vehicleNumber() const { return _vehicleNumber; }
        auto arrivalTime()          const { return _arrivalTime;   }
        auto busStopCount()         const { return _busStopCount;  }
        bool isLast()               const { return _isLast;        }

        void setVehicleType(const std::wstring& vt)   { _vehicleType = vt;   }
        void setVehicleState(const std::wstring& vs)  { _vehicleState = vs;  }
        void setVehicleNumber(const std::wstring& vn) { _vehicleNumber = vn; }
        void setArrivalTime(int32_t at)               { _arrivalTime = at;   }
        void setBusStopCount(int32_t bsc)             { _busStopCount = bsc; }
        void setIsLast(bool il)                       { _isLast = il;        }

    private:
        std::wstring _vehicleType;
        std::wstring _vehicleState;
        std::wstring _vehicleNumber;
        int32_t _arrivalTime;
        int32_t _busStopCount;
        bool _isLast;
    };

    ////////////////////////////////////////////////////////////////////////

    class LineArrivalInfo {
    public:
        const auto& lineId()   const { return _lineId;   }
        const auto& lineName() const { return _lineName; }
        const auto& arrivals() const { return _arrivals; }

        void setLineId(const std::wstring& li)   { _lineId = li;             }
        void setLineName(const std::wstring& ln) { _lineName = ln;           }
        void addArrival(BusArrivalInfo& bai)     { _arrivals.push_back(bai); }

    private:
        std::wstring _lineId;
        std::wstring _lineName;

        std::vector<BusArrivalInfo> _arrivals;
    };

    ////////////////////////////////////////////////////////////////////////

    class StopInfo {
    public:
        StopInfo()
            : _lastUpdate(std::chrono::system_clock::now())
        {
        }

    public:
        const auto& id()           const { return _id;           }
        const auto& name()         const { return _name;         }
        const auto& lineArrivals() const { return _lineArrivals; }

        void setId(const std::wstring& id)     { _id = id;     }
        void setId(std::wstring&& id)          { _id = id;     }
        void setName(const std::wstring& name) { _name = name; }
        void setName(std::wstring&& name)      { _name = name; }

        inline void touchLastUpdate();
        inline void addLineArrival(const std::wstring& busNumber, LineArrivalInfo& lai);

    private:
        std::wstring _id;
        std::wstring _name;
        std::chrono::time_point<std::chrono::system_clock> _lastUpdate;

        std::map<std::wstring, LineArrivalInfo> _lineArrivals;  // key = bus number
    };

    inline void StopInfo::touchLastUpdate()
    {
        _lastUpdate = std::chrono::system_clock::now();
    }

    inline void StopInfo::addLineArrival(const std::wstring& busNumber, LineArrivalInfo& lai)
    {
        //_lineArrivals[busNumber].swap(lai);
        _lineArrivals.emplace(std::make_pair(busNumber, lai));
    }
}


#endif // _KUDD_BUS_REMINDER_DATA_TYPES_H_

// 끝.

