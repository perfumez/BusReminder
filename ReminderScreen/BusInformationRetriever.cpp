#include "BusInformationRetriever.h"
#include "DataTypes.h"
#include <sstream>
#include <codecvt>
#include <boost/locale/encoding_utf.hpp>
#include <boost/asio.hpp>
#include <rapidjson/document.h>


using namespace kudd;

// 삼송교 : http://map.daum.net/bus/stop.json?callback=kudd&busstopid=31101581021


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

    const std::wstring CALLBACKFUNC(L"kudd");
}

BusInformationRetriever::BusInformationRetriever()
    : _quitRequest(false)
{
    //retrieve("map.daum.net", "/bus/stop.json?callback=" + CALLBACKFUNC + "&busstopid=BS263681");
    //retrieve("map.daum.net", "/bus/stop.json?callback=" + CALLBACKFUNC + "&busstopid=31101581021");

    //_stopsToWatch[SAMSONGGYO] = L"/bus/stop.json?callback=" + CALLBACKFUNC + L"&busstopid=31101581021";

    std::thread t(std::bind(&BusInformationRetriever::threadFunc, this));
    _thread.swap(t);
}

BusInformationRetriever::~BusInformationRetriever()
{
    cleanup();
}

BusInformationRetriever& BusInformationRetriever::get()
{
    static BusInformationRetriever instance;
    return instance;
}

void BusInformationRetriever::cleanup()
{
    _quitRequest = true;
    if (_thread.joinable()) {
        _thread.join();
    }
}

void BusInformationRetriever::addRetrieveInfo(const std::wstring& busStopId, const std::set<std::wstring>& busLines)
{
    std::lock_guard<decltype(_mutex)> lg(_mutex);
    _stopsToWatch[busStopId] = L"/bus/stop.json?callback=" + CALLBACKFUNC + L"&busstopid=" + busStopId;
    _numbersToWatch[busStopId] = busLines;
}

std::shared_ptr<StopInfo> BusInformationRetriever::getStopInfo(const std::wstring& id)
{
    std::lock_guard<decltype(_mutex)> lg(_mutex);
    const auto where = _stopInfoMap.find(id);
    return where == _stopInfoMap.end() ? std::shared_ptr<StopInfo>() : where->second;
}

void BusInformationRetriever::threadFunc()
{
    while (!_quitRequest) {
        bool hasItem = false;
        {
            std::lock_guard<decltype(_mutex)> lg(_mutex);
            for (const auto& each : _stopsToWatch) {
                auto parsed = retrieve("map.daum.net", wstring_to_utf8(each.second));
                parsed->touchLastUpdate();
                _stopInfoMap[parsed->id()] = parsed;
                hasItem = !_stopInfoMap.empty();
            }
        }

        if (hasItem) {
            int32_t sleepCount = 10 * 15;
            while (!_quitRequest && --sleepCount > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
}

std::shared_ptr<StopInfo> BusInformationRetriever::retrieve(const std::string& host, const std::string& path)
{
    std::shared_ptr<StopInfo> parsed;

    try {
        using boost::asio::ip::tcp;
        boost::asio::io_service io_service;

        // Get a list of endpoints corresponding to the server name.
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(host, "http");
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        // Try each endpoint until we successfully establish a connection.
        tcp::socket socket(io_service);
        boost::asio::connect(socket, endpoint_iterator);

        // Form the request. We specify the "Connection: close" header so that the
        // server will close the socket after transmitting the response. This will
        // allow us to treat all data up until the EOF as the content.
        boost::asio::streambuf request;
        std::ostream request_stream(&request);
        request_stream << "GET " << path << " HTTP/1.0\r\n";
        request_stream << "Host: " << host << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Connection: close\r\n\r\n";

        // Send the request.
        boost::asio::write(socket, request);

        // Read the response status line. The response streambuf will automatically
        // grow to accommodate the entire line. The growth may be limited by passing
        // a maximum size to the streambuf constructor.
        boost::asio::streambuf response;
        boost::asio::read_until(socket, response, "\r\n");

        // Check that response is OK.
        std::istream response_stream(&response);
        std::string http_version;
        response_stream >> http_version;
        unsigned int status_code;
        response_stream >> status_code;
        std::string status_message;
        std::getline(response_stream, status_message);
        if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
            return std::shared_ptr<StopInfo>();
        }
        if (status_code != 200) {
            return std::shared_ptr<StopInfo>();
        }

        // Read the response headers, which are terminated by a blank line.
        boost::asio::read_until(socket, response, "\r\n\r\n");

        // Process the response headers.
        std::vector<std::string> headers;
        std::string header;
        while (std::getline(response_stream, header) && header != "\r") {
            headers.emplace_back(header);
        }

        // Write whatever content we already have to output.
        std::ostringstream body;
        if (response.size() > 0) {
            body << &response;
        }

        // Read until EOF, writing data to output as we go.
        boost::system::error_code error;
        while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error)) {
            body << &response;
        }

        if (error != boost::asio::error::eof) {
            throw boost::system::system_error(error);
        }

        std::string&& str = body.str();
        //std::wstring&& ws = utf8_to_wstring(str);

        parsed = parse(str);
    }
    catch (std::exception&) {
    }

    return parsed;
}

namespace {
    LineArrivalInfo parseLineArrival(const rapidjson::GenericValue<rapidjson::UTF8<> >& gv)
    {
        LineArrivalInfo lii;

        auto itr = gv.MemberBegin();
        for (; itr != gv.MemberEnd(); ++itr) {
            std::string name = itr->name.GetString();

            if (name == "id") {
                lii.setLineId(utf8_to_wstring(itr->value.GetString()));
            }
            else if (name == "name") {
                lii.setLineName(utf8_to_wstring(itr->value.GetString()));
            }
            else if (name == "arrival") {
                BusArrivalInfo bai, bai2;
                auto itrArr = itr->value.MemberBegin();
                for (; itrArr != itr->value.MemberEnd(); ++itrArr) {
                    std::string innerName = itrArr->name.GetString();
                    if (innerName == "vehicleNumber") {
                        bai.setVehicleNumber(utf8_to_wstring(itrArr->value.GetString()));
                    }
                    else if (innerName == "vehicleType") {
                        bai.setVehicleType(utf8_to_wstring(itrArr->value.GetString()));
                    }
                    else if (innerName == "vehicleState") {
                        bai.setVehicleState(utf8_to_wstring(itrArr->value.GetString()));
                    }
                    else if (innerName == "arrivalTime") {
                        bai.setArrivalTime(itrArr->value.GetInt());
                    }
                    else if (innerName == "lastVehicle") {
                        bai.setIsLast(itrArr->value.GetBool());
                    }
                    else if (innerName == "busStopCount") {
                        bai.setBusStopCount(itrArr->value.GetInt());
                    }
                    else if (innerName == "vehicleNumber2") {
                        bai2.setVehicleNumber(utf8_to_wstring(itrArr->value.GetString()));
                    }
                    else if (innerName == "vehicleType2") {
                        bai2.setVehicleType(utf8_to_wstring(itrArr->value.GetString()));
                    }
                    else if (innerName == "vehicleState2") {
                        bai2.setVehicleState(utf8_to_wstring(itrArr->value.GetString()));
                    }
                    else if (innerName == "arrivalTime2") {
                        bai2.setArrivalTime(itrArr->value.GetInt());
                    }
                    else if (innerName == "lastVehicle2") {
                        bai2.setIsLast(itrArr->value.GetBool());
                    }
                    else if (innerName == "busStopCount2") {
                        bai2.setBusStopCount(itrArr->value.GetInt());
                    }
                }
                lii.addArrival(bai);
                lii.addArrival(bai2);
            }
        }

        return lii;
    }
}

std::shared_ptr<StopInfo> BusInformationRetriever::parse(const std::string& json)
{
    std::string&& j = json.substr(CALLBACKFUNC.size() + 1, json.size() - CALLBACKFUNC.size() - 2);
    std::wstring&& wj = utf8_to_wstring(j);

    using namespace rapidjson;

    Document document;
    document.Parse<0>(j.c_str());

    if (document.HasParseError()) {
        throw std::runtime_error("Json Parse Error!!");
    }

    std::shared_ptr<StopInfo> si(new StopInfo());

    auto itr = document.MemberBegin();
    for (; itr != document.MemberEnd(); ++itr) {
        std::string name = itr->name.GetString();

        if (name == "id") {
            si->setId(utf8_to_wstring(itr->value.GetString()));
        }
        else if (name == "name") {
            si->setName(utf8_to_wstring(itr->value.GetString()));
        }
        else if (name == "lines") {
            auto arrItr = itr->value.Begin();
            for (; arrItr != itr->value.End(); ++arrItr) {
                LineArrivalInfo&& lii = parseLineArrival(*arrItr);
                si->addLineArrival(lii.lineName(), lii);
            }
        }
    }

    return si;
}

// 끝.
