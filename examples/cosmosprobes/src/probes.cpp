#include <print>
#include <format>

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>


#include "nlohmann/json.hpp"
#include "siddiqsoft/restcl.hpp"


int main(int argc, char** argv)
{
    using namespace siddiqsoft::restcl_literals;

    std::atomic_bool done = false;
    std::println(std::cerr, "{} - Init the CurlLib singleton.\n", __func__);
    auto myCurlInstance = siddiqsoft::LibCurlSingleton::GetInstance();
    if (myCurlInstance) {
        auto wrc = siddiqsoft::GetRESTClient();

        wrc->configure({{"connectTimeout", 3000}, // timeout for the connect phase
                        {"timeout", 5000},        // timeout for the overall IO phase
                        {"trace", true}});
        auto req = siddiqsoft::rest_request("https://lws2.siddiq.org:8081/"_GET);
        auto resp = wrc->send(req);
        if (resp && resp->success()) {
            std::println(std::cerr, "  - Got Valid Response ------ \n{}", *resp);
        }
        else if (resp) {
            auto [ec, emsg] = resp->status();
            std::println(std::cerr, "  - Got response error: {} - {}", ec, emsg);
        }
        else {
            std::println(std::cerr, "  - Got error: `{}` -- `{}`", resp.error(), strerror(resp.error()));
        }

        // done = true;
        // done.notify_all();

        // done.wait(false);
        return 0;
    }
    else {
        std::println(std::cerr, "{} - Failed to get CurlLib singleton instance!", __func__);
        return 1;
    }
}