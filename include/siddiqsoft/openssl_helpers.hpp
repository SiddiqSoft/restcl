#pragma once
#ifndef OPENSSL_HELPERS_HPP
#define OPENSSL_HELPERS_HPP
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <chrono>
#include <sstream>
#include <atomic>
#include <mutex>

#include "openssl/ssl.h"
#include "openssl/err.h"

namespace siddiqsoft
{

    class OpenSSLSingleton
    {
    private:
        std::once_flag isInitialized {};

    public:
        OpenSSLSingleton()                   = default;

        OpenSSLSingleton(OpenSSLSingleton&&) = delete;
        auto operator=(OpenSSLSingleton&&)   = delete;

        ~OpenSSLSingleton() { }
        
        auto configure() -> OpenSSLSingleton& { return *this; }

        auto start() -> OpenSSLSingleton&
        {
            std::call_once(isInitialized, [&]() {
                // Perform once-per-application OpenSSL initialization logic
                /* Setup encryption algorithms and load error strings */
                OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS,NULL);
            });

            return *this;
        }

        /**
         * @brief Get a new SSL context.
         * Auto-clears the SSL_CTX when this object goes out of scope.
         * @return std::unique_ptr<SSL_CTX> 
         */
        [[nodiscard("Auto-clears the SSL_CTX when this object goes out of scope.")]] auto getCTX()
        {
            if (std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)> ctx(SSL_CTX_new(TLS_method()), &SSL_CTX_free); ctx) {
                // Set options..
                SSL_CTX_set_options(ctx.get(), SSL_OP_ALL);
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
                //	21May2009 MAAS - Do not disable this option; we need it for Windows.
                SSL_CTX_set_options(ctx.get(), SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER);
#endif
                return ctx;
            }

            // Failure means nothing/invalid object is returned!
            throw std::runtime_error("Something went wrong with OpenSSL");
        };

    protected:
        void __cdecl thread_setup() {};
        void __cdecl thread_cleanup() {};
    };
} // namespace siddiqsoft

#endif