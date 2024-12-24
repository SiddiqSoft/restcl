/*
    restcl : Focussed REST Client for Modern C++

    BSD 3-Clause License

    Copyright (c) 2021, Siddiq Software
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.

    3. Neither the name of the copyright holder nor the names of its
       contributors may be used to endorse or promote products derived from
       this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, rrd, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#ifndef BASIC_RESTCLIENT_HPP
#define BASIC_RESTCLIENT_HPP


#include <functional>

#include "restcl_definitions.hpp"
#include "basic_request.hpp"
#include "basic_response.hpp"

namespace siddiqsoft
{
    /// @brief The function or lambda must accept const basic_request& and const basic_response&
    using basic_callbacktype = std::function<void(const basic_request&, const basic_response&)>;


    /// @brief Base class for the rest client
    class basic_restclient
    {
    public:
        /// @brief Synchronous implementation of the IO
        /// @param req Request
        /// @return The response
        [[nodiscard]] virtual basic_response send(basic_request&) = 0;

        /// @brief Support for async callback
        /// @param req Request
        /// @param callback function that accepts const basic_restrequst& and const basic_response&
        virtual void send(basic_request&&, basic_callbacktype&) = 0;

        /// @brief Support for async callback
        /// @param req Request
        /// @param callback function that accepts const basic_restrequst& and const basic_response&
        virtual void send(basic_request&&, basic_callbacktype&&) = 0;
    };

    /**
     * @brief Container to store the basic_request and the optional callback.
     *
     */
    struct RestPoolArgsType
    {
        RestPoolArgsType(basic_request&& r, basic_callbacktype& cb)
            : request(std::move(r)) // own the request
            , callback(cb)          // make a copy
        {
        }

        RestPoolArgsType(basic_request&& r, basic_callbacktype&& cb)
            : request(std::move(r))   // own the request
            , callback(std::move(cb)) // own the callback
        {
        }

        /**
         * @brief Represents the basic_request object
         *
         */
        basic_request request;

        /**
         * @brief Holds the callback to the client code
         *
         */
        basic_callbacktype callback {};
    };

} // namespace siddiqsoft

#endif // !BASIC_RESTCLIENT_HPP
