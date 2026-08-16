#include <print>
#include <format>

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "nlohmann/json.hpp"
#include "siddiqsoft/ScopeTrace.hpp"
#include "siddiqsoft/restcl.hpp"

static auto& Log = siddiqsoft::ScopeTrace::GetInstance("cosmosprobes", siddiqsoft::LogLevel::trace);

int main(int argc, char** argv)
{
    using namespace siddiqsoft::restcl_literals;

    std::atomic_bool done = false;
    Log.info("{} - Init the CurlLib singleton.\n", __func__);
    auto myCurlInstance = siddiqsoft::LibCurlSingleton::GetInstance();
    if (myCurlInstance) {
        auto wrc = siddiqsoft::GetRESTClient();

        wrc->configure({{"connectTimeout", 3000}, // timeout for the connect phase
                        {"timeout", 5000},        // timeout for the overall IO phase
                        {"trace", false}});

        // The port 8080 is for checking the health of the service.
        auto req  = siddiqsoft::rest_request("http://localhost:8080/ready"_GET);
        auto resp = wrc->send(req);
        if (resp && resp->success()) {
            Log.trace("  - Got Valid Response ------ \n{}", *resp);
        }
        else if (resp) {
            auto [ec, emsg] = resp->status();
            Log.info("  - Got response error: {} - {}", ec, emsg);
        }
        else {
            Log.warn("  - Got error: `{}` -- `{}`", resp.error(), curl_easy_strerror(static_cast<CURLcode>(resp.error())));
        }

        return 0;
    }
    else {
        Log.info("Failed to get CurlLib singleton instance!");
        return 1;
    }
}