#ifndef HTTP2JSON_HPP
#define HTTP2JSON_HPP

#include <string>

#include "rest_request.hpp"
#include "rest_response.hpp"

#include "single-header/ctre.hpp"


namespace siddiqsoft
{

    class http2json
    {
    private:
        static [[nodiscard]] auto parseStartLine(siddiqsoft::rest_response& resp, std::string& srcBuffer) -> std::string&
        {
            if (auto result = ctre::match<"(POST|CONNECT|GET|INFO)\\s(.*)\\s(HTTP.*)">(srcBuffer); result) {
                resp.setStatus(result.get<1>(), result.get<2>());

                // advance the buffer
                srcBuffer.erase(0, result.get<0>().length());
                return srcBuffer;
            }
        }

    public:
        static [[nodiscard]] auto parse(std::string& srcBuffer) -> siddiqsoft::rest_response
        {
            siddiqsoft::rest_response resp {};

            parseStartLine(resp, srcBuffer);
            return resp;
            //return srcBuffer;
        }
    }
} // namespace siddiqsoft

#endif