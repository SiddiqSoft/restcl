/**
 * @file openssl_helpers.hpp
 * @author Abdulkareem Siddiq (github@siddiqsoft.com)
 * @brief OpenSSL v3.0 Helpers
 * @version 0.1
 * @date 2024-12-23
 * 
 * @copyright Copyright (c) 2024 SiddiqSoftware
 * 
 */

#pragma once
#ifndef OPENSSL_HELPERS_HPP
#define OPENSSL_HELPERS_HPP
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>


#include "openssl/err.h"
#include "openssl/ssl.h"

namespace siddiqsoft
{
    /**
     * @brief LibSSLSingleton provides entry point and stores the various configuration for your
     *        applications use of the OpenSSL library.
     *        There is no need to perform explicit initialization and cleanup for the library.
     *        https://docs.openssl.org/3.0/man7/migration_guide/#support-of-legacy-engines
     */
    class LibSSLSingleton
    {
    private:
        std::once_flag initFlag {};

    public:
        LibSSLSingleton()                   = default;

        LibSSLSingleton(LibSSLSingleton&&) = delete;
        auto operator=(LibSSLSingleton&&)   = delete;

        ~LibSSLSingleton() { }

        auto configure() -> LibSSLSingleton& { return *this; }

        auto start() -> LibSSLSingleton&
        {
            std::call_once(initFlag, [&]() {
                // Perform once-per-application OpenSSL initialization logic
                /* Setup encryption algorithms and load error strings */
                OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS, NULL);
                isInitialized = true;
            });

            return *this;
        }

        /**
         * @brief Get a new SSL context.
         * Auto-clears the SSL_CTX when this object goes out of scope.
         * @return std::shared_ptr<SSL_CTX>
         */
        [[nodiscard("Auto-clears the SSL_CTX when this object goes out of scope.")]] auto getCTX()
        {
            if (std::shared_ptr<SSL_CTX> ctx(SSL_CTX_new(TLS_method()), SSL_CTX_free); ctx) {
                // Set options..
                SSL_CTX_set_options(ctx.get(), SSL_OP_ALL);
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
                //	21May2009 MAAS - Do not disable this option; we need it for
                // Windows.
                SSL_CTX_set_options(ctx.get(), SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER);
#endif
                return ctx;
            }

            // Failure means nothing/invalid object is returned!
            throw std::runtime_error("Something went wrong with OpenSSL");
        };

#if defined(DEBUG) || defined(_DEBUG)
    public:
        bool isInitialized {false};
#else
    private:
        bool isInitialized {false};
#endif
    };


    /**
     * @brief LibCryptoSingleton provides a facility to centrally manage the lifespan of the various
     *        OpenSSL crypto context and configuration objects.
     * 
     */
    class LibCryptoSingleton
    {
    private:
        std::once_flag initFlag {};

    public:
        LibCryptoSingleton()                   = default;

        LibCryptoSingleton(LibCryptoSingleton&&) = delete;
        auto operator=(LibCryptoSingleton&&)   = delete;

        ~LibCryptoSingleton() { }

        auto configure() -> LibCryptoSingleton& { return *this; }

        auto start() -> LibCryptoSingleton&
        {
            std::call_once(initFlag, [&]() {
                /* Setup encryption algorithms and load error strings */
                isInitialized = true;
            });

            return *this;
        }

#if defined(DEBUG) || defined(_DEBUG)
    public:
        bool isInitialized {false};
#else
    private:
        bool isInitialized {false};
#endif
    };
} // namespace siddiqsoft

#endif