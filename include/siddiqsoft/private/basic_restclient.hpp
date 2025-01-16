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
#include <optional>
#ifndef BASIC_RESTCLIENT_HPP
#define BASIC_RESTCLIENT_HPP

#include <tuple>
#include <functional>
#include <expected>

#include "http_frame.hpp"
#include "rest_request.hpp"
#include "rest_response.hpp"

namespace siddiqsoft
{
    struct rest_result_error;
    
    /// @brief The function or lambda must accept const rest_request& and const rest_response&
    using basic_callbacktype = std::function<void(rest_request&, std::expected<rest_response, int>)>;


    /// @brief Base class for the rest client
    class basic_restclient
    {
    public:
        /**
         * @brief Configuration entry point. This function maybe used for initializing ssl objects
         *        and/or other platform-specific shared resources.
         *
         * @param us The User-Agent string
         * @param ... Variable number of arguments depending on the platform.
         *            One of the variable arguments would be the callback for sendAsync operations.
         * @return basic_restclient& Returns reference to self to allow chaining
         */
        virtual basic_restclient& configure(const nlohmann::json& ={}, basic_callbacktype&& = {}) = 0;

        /// @brief Synchronous implementation of the IO
        /// @param req Request
        /// @return The response
        [[nodiscard]] virtual std::expected<rest_response, int> send(rest_request&) = 0;

        /// @brief Asynchronous operation. The callback must be provided here or previously via the configure()
        /// @param req Request
        /// @param callback Optional callback function.
        ///                 If not present then the one provided during configuration is used.
        ///                 If no callback has been registered or provided here then an invalid_argument
        ///                 exception should be thrown.
        virtual basic_restclient& sendAsync(rest_request&&, basic_callbacktype&& = {}) = 0;
    };

    /**
     * @brief Container to store the rest_request and the optional callback.
     *
     */
    struct RestPoolArgsType
    {
        RestPoolArgsType(rest_request&& r, basic_callbacktype& cb)
            : request(std::move(r)) // own the request
            , callback(cb)          // make a copy
        {
        }

        RestPoolArgsType(rest_request&& r, basic_callbacktype&& cb)
            : request(std::move(r))   // own the request
            , callback(std::move(cb)) // own the callback
        {
        }

        rest_request request;
        basic_callbacktype callback {};
    };

} // namespace siddiqsoft


#endif // !BASIC_RESTCLIENT_HPP
