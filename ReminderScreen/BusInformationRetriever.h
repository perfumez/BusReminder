#ifndef _KUDD_BUS_INFORMATION_RETRIEVER_H_
#define _KUDD_BUS_INFORMATION_RETRIEVER_H_

#include <thread>
#include <string>
#include <map>
#include <set>
#include <memory>
#include <mutex>
#include <atomic>

namespace kudd {
    class StopInfo;
    class BusInformationRetriever {
    private:
        BusInformationRetriever();

    public:
        static BusInformationRetriever& get();
        ~BusInformationRetriever();

    public:
        void cleanup();

    public:
        void addRetrieveInfo(const std::wstring& busStopId, const std::set<std::wstring>& busLines);
        std::shared_ptr<StopInfo> getStopInfo(const std::wstring& id);

    private:
        std::shared_ptr<StopInfo> retrieve(const std::string& host, const std::string& path);
        std::shared_ptr<StopInfo> parse(const std::string& json);

    private:
        void threadFunc();

    private:
        std::thread _thread;
        std::recursive_mutex _mutex;
        std::atomic_bool _quitRequest;

        using StopInfoMap = std::map<std::wstring, std::shared_ptr<StopInfo> >;
        using StopsToWatch = std::map<std::wstring, std::wstring>;              // key : stopId, value : url
        using NumbersToWatch = std::map<std::wstring, std::set<std::wstring> >; // key stopId, value : bus Line

        StopInfoMap _stopInfoMap;
        StopsToWatch _stopsToWatch;
        NumbersToWatch _numbersToWatch;
    };
}

#endif //_KUDD_BUS_INFORMATION_RETRIEVER_H_

// 끝.