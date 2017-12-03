#include "Settings.h"
#include <codecvt>
#include <boost/locale/encoding_utf.hpp>

using namespace kudd;

namespace {
#if 0
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
#else
    // GCC 에서는 wstring_convert 를 못쓰는듯 하다..
    // https://stackoverflow.com/questions/15615136/is-codecvt-not-a-std-header
    std::wstring utf8_to_wstring(const std::string& str)
    {
        return boost::locale::conv::utf_to_utf<wchar_t>(str.c_str(), str.c_str() + str.size());
    }

    std::string wstring_to_utf8(const std::wstring& str)
    {
        return boost::locale::conv::utf_to_utf<char>(str.c_str(), str.c_str() + str.size());
    }
#endif

    SubscribeInfo createSubcription(SubscribeInfo::Direction dir, const std::wstring& stopName,
                                    const std::wstring& stopId, std::pair<uint16_t, uint16_t>&& time)
    {
        SubscribeInfo si;
        si.setDirection(dir);
        si.setStopName(stopName);
        si.setStopId(stopId);
        si.setTime(std::move(time));
        //si.setBusLines(std::move(lines));

        return si;
    }

    const std::wstring GUPABAL(utf8_to_wstring("\xea\xb5\xac\xed\x8c\x8c\xeb\xb0\x9c"));

    const std::wstring GUPABAL_ID_TO_GOYANG(L"BS14360");
    const std::wstring GUPABAL_ID_TO_SEOUL(L"BS14359");
}

Configuration& Configuration::get()
{
    static Configuration __instance__;
    return __instance__;
}

Configuration::Configuration()
{
    makeDefault();
}

bool Configuration::loadFrom(const std::wstring& filename)
{
    return true;
}

bool Configuration::saveTo(const std::wstring& filename) const
{
    return false;
}

void Configuration::makeDefault()
{
    _subscriptions.clear();

    // 구파발 -> 고양방향.
    _subscriptions[GUPABAL_ID_TO_GOYANG] = createSubcription(SubscribeInfo::WEST, GUPABAL, GUPABAL_ID_TO_GOYANG, { 450, 480 });
    auto& stop1 = _subscriptions[GUPABAL_ID_TO_GOYANG];
    stop1.addBusLine(L"9703", BusDisplayOption(L"ff0000", L"ffffff"));
    stop1.addBusLine(L"773", BusDisplayOption(L"0000ff", L"ffffff"));

    // 구파발 -> 서울방향.
    _subscriptions[GUPABAL_ID_TO_SEOUL] = createSubcription(SubscribeInfo::EAST, GUPABAL, GUPABAL_ID_TO_SEOUL, { 450, 480 });
    auto& stop2 = _subscriptions[GUPABAL_ID_TO_SEOUL];
    stop2.addBusLine(L"9703", BusDisplayOption(L"ff0000", L"ffffff"));
    stop2.addBusLine(L"773", BusDisplayOption(L"0000ff", L"ffffff"));
}

// 끝.
