/**
 * @file basic_restclient.hpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2024-12-24
 * 
 * @copyright Copyright (c) 2024 Siddiq Software
 * 
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
