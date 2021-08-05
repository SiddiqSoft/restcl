/*
	restcl : PROJECTDESCRIPTION

	BSD 3-Clause License

	Copyright (c) 2021, Siddiq Software LLC
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
#ifndef RESTREQUESTTYPE_HPP
#define RESTREQUESTTYPE_HPP

#include <chrono>
#include <string>
#include <functional>
#include <memory>
#include <format>
#include <iterator>

#include "nlohmann/json.hpp"
#include "siddiqsoft/SplitUri.hpp"


namespace siddiqsoft
{
	/// @brief Create date as specified by RFC1123
	/// @return RFC1123 specified date. Note that "GMT" here is treated as "UTC"
	static std::string Date_rfc1123()
	{
		// https://www.rfc-editor.org/info/rfc1123; https://www.rfc-editor.org/rfc/rfc1123.txt
		// https://docs.microsoft.com/en-us/dotnet/standard/base-types/standard-date-and-time-format-strings#RFC1123
		// std::Format: Thu, 10 Apr 2008 13:30:00.0000000 GMT
		//return std::format("{:%a, %d %b %Y %H:%M:%S GMT}", std::chrono::utc_clock::now());

		// Current version of the std::chrono formatters return nanoseconds which is not compliant
		char buff[sizeof "Tue, 01 Nov 1994 08:12:31 GMT"] {};
		tm   timeInfo {};
		auto rawtime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		// Get the UTC time packet.
		if (gmtime_s(&timeInfo, &rawtime) != EINVAL) strftime(buff, sizeof(buff), "%a, %d %h %Y %T GMT", &timeInfo);

		return buff;
	}


	enum class RESTMethodType
	{
		Get,
		Patch,
		Post,
		Put,
		Delete,
		Info
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(RESTMethodType,
	                             {{RESTMethodType::Get, "GET"},
	                              {RESTMethodType::Patch, "PATCH"},
	                              {RESTMethodType::Put, "PUT"},
	                              {RESTMethodType::Post, "POST"},
	                              {RESTMethodType::Delete, "DELETE"},
	                              {RESTMethodType::Info, "INFO"}});


	/// @brief A REST request utility class. Models the request a JSON document with `request`, `headers` and `content` elements.
	class RESTRequestType
	{
	public:
		/// @brief Constructor with endpoint and optional headers and json content
		/// @param endpoint Fully qualified http schema uri
		/// @param h Optional json containing the headers
		RESTRequestType(const RESTMethodType& reqMethod, const std::string& endpoint, const nlohmann::json& h = nullptr)
		{
			uriHttp = SplitUri(endpoint);

			if (h.is_object() && !h.is_null()) { rrd["headers"] = h; }

			rrd["request"]["uri"]    = uriHttp.url;
			rrd["request"]["method"] = reqMethod;

			// Enforce some default headers
			auto& hs = rrd.at("headers");
			if (!hs.contains("Date")) hs["Date"] = Date_rfc1123();
			if (!hs.contains("Accept")) hs["Accept"] = "application/json";
			if (!hs.contains("Host")) hs["Host"] = std::format("{}:{}", uriHttp.authority.host, uriHttp.authority.port);
			if (!hs.contains("Content-Length")) hs["Content-Length"] = 0;
			if (!hs.contains("User-Agent")) hs["User-Agent"] = "siddiqsoft/restcl";
		}

		RESTRequestType(const RESTMethodType& reqMethod,
		                const std::string&    endpoint,
		                const nlohmann::json& h,
		                const nlohmann::json& c)
			: RESTRequestType(reqMethod, endpoint, h)
		{
			setContent(c);
		}


		explicit RESTRequestType(const RESTMethodType& reqMethod,
		                         const std::string&    endpoint,
		                         const nlohmann::json& h,
		                         const std::string&    c)
			: RESTRequestType(reqMethod, endpoint, h)
		{
			if (h.is_null() || (h.is_object() && !h.contains("Content-Type"))) rrd["headers"]["Content-Type"] = "text/plain";
			rrd["headers"]["Content-Length"] = c.length();
			rrd["content"]                   = c;
		}

		explicit RESTRequestType(const RESTMethodType& reqMethod,
		                         const std::string&    endpoint,
		                         const nlohmann::json& h,
		                         const char*           c)
			: RESTRequestType(reqMethod, endpoint, h)
		{
			if (h.is_null() || (h.is_object() && !h.contains("Content-Type"))) rrd["headers"]["Content-Type"] = "text/plain";
			rrd["headers"]["Content-Length"] = strlen(c);
			rrd["content"]                   = c;
		}


		/// @brief Set the Uri via the endpoint
		/// @param e Fully qualified endpoint uri with http scheme
		/// @return Self
		auto& setEndpoint(const std::string& e)
		{
			uriHttp = SplitUri(e);
			return *this;
		}

		/// @brief Returns the mutable reference to the headers json
		/// @return Json reference to headers element
		nlohmann::json& headers() { return rrd.at("headers"); }
		auto&           getHeaders() const { return rrd.at("headers"); }

		/// @brief Returns the mutable reference to the request json
		/// @return Json reference to request element
		nlohmann::json& request() { return rrd.at("request"); }
		auto&           getRequest() const { return rrd.at("request"); }

		/// @brief Return the parsed endpoint into Uri
		/// @return Uri structure
		auto uri() const { return uriHttp; }


		/// @brief Set the content (non-JSON)
		/// @param ctype Content-Type
		/// @param c The content
		/// @return Self
		RESTRequestType& setContent(const std::string& ctype, const std::string& c)
		{
			rrd["headers"]["Content-Type"]   = ctype;
			rrd["headers"]["Content-Length"] = c.length();
			rrd["content"]                   = c;
			return *this;
		}

		/// @brief Set the content to json
		/// @param c JSON content
		/// @return Self
		RESTRequestType& setContent(const nlohmann::json& c)
		{
			rrd["content"]                   = c;
			rrd["headers"]["Content-Length"] = c.dump().length();
			rrd["headers"]["Content-Type"]   = "application/json";
			return *this;
		}


		std::string getContent() const
		{
			// Build the content to ensure we have the content-type
			if (rrd["content"].is_object()) { return rrd["content"].dump(); }
			else if (rrd["content"].is_string())
			{
				return rrd["content"].get<std::string>();
			}

			return {};
		}


		/// @brief Encode the headers to the given argument
		/// @param rs String where the headers is "written-to".
		void encodeHeaders_to(std::string& rs) const
		{
			for (auto& [k, v] : rrd["headers"].items())
			{
				if (v.is_string()) { std::format_to(std::back_inserter(rs), "{}: {}\r\n", k, v.get<std::string>()); }
				else if (v.is_number_unsigned())
				{
					std::format_to(std::back_inserter(rs), "{}: {}\r\n", k, v.get<uint64_t>());
				}
				else if (v.is_number_integer())
				{
					std::format_to(std::back_inserter(rs), "{}: {}\r\n", k, v.get<int>());
				}
				else
				{
					std::format_to(std::back_inserter(rs), "{}: {}\r\n", k, v.dump());
				}
			}

			std::format_to(std::back_inserter(rs), "\r\n");
		}

		/// @brief Encode the request to the given argument via the back_inserter
		/// @param rs String where the request is "written-to".
		void encode_to(std::string& rs) const
		{
			std::string body;
			auto&       hs = rrd.at("headers");
			auto&       rl = rrd.at("request");

			// Request Line
			std::format_to(std::back_inserter(rs),
			               "{} {} {}\r\n",
			               rl["method"].get<std::string>(),
			               rl["uri"].get<std::string>(),
			               rl["version"].get<std::string>());

			// Build the content to ensure we have the content-type
			if (rrd["content"].is_object()) { body = rrd["content"].dump(); }
			else if (rrd["content"].is_string())
			{
				body = rrd["content"].get<std::string>();
			}

			// Headers..
			encodeHeaders_to(rs);

			// Finally the content..
			if (!body.empty()) { std::format_to(std::back_inserter(rs), "{}", body); }
		}

		/// @brief Encode the request to a byte stream ready to transfer to the remote server.
		/// @return String
		std::string encode() const
		{
			std::string rs;
			encode_to(rs);
			return std::move(rs);
		}

	public:
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(RESTRequestType, uriHttp, sendAttempt, rrd);
		friend std::ostream& operator<<(std::ostream&, const RESTRequestType&);

	protected:
		Uri<std::string> uriHttp;
		uint16_t         sendAttempt {1};
		nlohmann::json   rrd {{"request", {{"method", RESTMethodType::Get}, {"uri", nullptr}, {"version", "HTTP/1.1"}}},
                            {"headers", nullptr},
                            {"content", nullptr}};
	};


	static std::ostream& operator<<(std::ostream& os, const RESTRequestType& src)
	{
		std::format_to(std::ostream_iterator<char>(os), "{}", src);
		return os;
	}


	/// @brief REST Response object
	class RESTResponseType
	{
	public:
		RESTResponseType() { }

		/// @brief Construct a response object based on the given transport error
		/// @param err Specifies the transport error.
		RESTResponseType(const std::pair<uint32_t, std::string>& err)
		{
			ioErrorCode = std::get<0>(err);
			ioError     = std::get<1>(err);
		}


		RESTResponseType& setStatusCode(uint16_t sc)
		{
			statusCode                = sc;
			rrd["response"]["status"] = statusCode;
			return *this;
		}

		RESTResponseType& setReason(const std::string& phrase)
		{
			rrd["response"]["reason"] = phrase;
			return *this;
		}

		RESTResponseType& setHeaders(const nlohmann::json& h)
		{
			if (h.is_object()) rrd["headers"] = h;
			return *this;
		}

		RESTResponseType& setContent(const std::string& c)
		{
			if (!c.empty())
			{
				try
				{
					rrd["content"] = nlohmann::json::parse(c);
					if (!rrd["headers"].contains("Content-Length")) rrd["headers"]["Content-Length"] = c.length();
					return *this;
				}
				catch (...)
				{
				}

				// We did not decode a json; assign as-is
				rrd["content"] = c;
				if (!rrd["headers"].contains("Content-Length")) rrd["headers"]["Content-Length"] = c.length();
			}

			return *this;
		}

		bool isSuccessful() const
		{
			if (ioErrorCode == 0)
			{
				if (statusCode > 99 && statusCode < 400) return true;
			}

			return false;
		}

		nlohmann::json& response() { return rrd.at("response"); }
		auto&           getResponse() const { return rrd.at("response"); }

		/// @brief Returns the mutable reference to the headers json
		/// @return Json reference to headers element
		nlohmann::json& headers() { return rrd.at("headers"); }
		auto&           getHeaders() const { return rrd.at("headers"); }


		/// @brief Encode the headers to the given argument
		/// @param rs String where the headers is "written-to".
		void encodeHeaders_to(std::string& rs) const
		{
			for (auto& [k, v] : rrd["headers"].items())
			{
				if (v.is_string()) { std::format_to(std::back_inserter(rs), "{}: {}\r\n", k, v.get<std::string>()); }
				else if (v.is_number_unsigned())
				{
					std::format_to(std::back_inserter(rs), "{}: {}\r\n", k, v.get<uint64_t>());
				}
				else if (v.is_number_integer())
				{
					std::format_to(std::back_inserter(rs), "{}: {}\r\n", k, v.get<int>());
				}
				else
				{
					std::format_to(std::back_inserter(rs), "{}: {}\r\n", k, v.dump());
				}
			}

			std::format_to(std::back_inserter(rs), "\r\n");
		}

		/// @brief Encode the request to the given argument via the back_inserter
		/// @param rs String where the request is "written-to".
		void encode_to(std::string& rs) const
		{
			std::string body;
			auto&       hs = rrd.at("headers");
			auto&       rl = rrd.at("response");

			// Request Line
			std::format_to(std::back_inserter(rs),
			               "{} {} {}\r\n",
			               rl["version"].get<std::string>(),
			               rl["status"].get<uint32_t>(),
			               rl["reason"].is_null() ? "" : rl["reason"].get<std::string>());

			// Build the content to ensure we have the content-type
			if (!rrd["content"].is_null() && rrd["content"].is_object()) { body = rrd["content"].dump(); }
			else if (!rrd["content"].is_null() && rrd["content"].is_string())
			{
				body = rrd["content"].get<std::string>();
			}

			// Headers..
			encodeHeaders_to(rs);

			// Finally the content..
			if (!body.empty()) { std::format_to(std::back_inserter(rs), "{}", body); }
		}

		/// @brief Encode the request to a byte stream ready to transfer to the remote server.
		/// @return String
		std::string encode() const
		{
			std::string rs;
			encode_to(rs);
			return std::move(rs);
		}

		std::tuple<uint32_t, const std::string&> getIOError() const { return {ioErrorCode, std::cref(ioError)}; }

	public:
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(RESTResponseType, statusCode, ioErrorCode, ioError, sendAttempt, rrd);
		friend std::ostream& operator<<(std::ostream&, const RESTResponseType&);

	protected:
		uint16_t       statusCode {0};
		uint32_t       ioErrorCode {0};
		std::string    ioError {};
		uint16_t       sendAttempt {1};
		nlohmann::json rrd {{"response", {{"status", statusCode}, {"reason", nullptr}, {"version", "HTTP/1.1"}}},
		                    {"headers", nullptr},
		                    {"content", nullptr}};
	};


	static std::ostream& operator<<(std::ostream& os, const RESTResponseType& src)
	{
		std::format_to(std::ostream_iterator<char>(os), "{}", src);
		return os;
	}
} // namespace siddiqsoft


/// @brief Formatters for the RESTMethodType
template <> struct std::formatter<siddiqsoft::RESTMethodType> : std::formatter<std::string>
{
	auto format(const siddiqsoft::RESTMethodType& sv, std::format_context& ctx)
	{
		return std::formatter<std::string>::format(nlohmann::json(sv).get<std::string>(), ctx);
	}
};


/// @brief Formatter for RESTRequestType
template <> struct std::formatter<siddiqsoft::RESTRequestType> : std::formatter<std::string>
{
	auto format(const siddiqsoft::RESTRequestType& sv, std::format_context& ctx)
	{
		return std::formatter<std::string>::format(sv.encode(), ctx);
	}
};


template <> struct std::formatter<siddiqsoft::RESTResponseType> : std::formatter<std::string>
{
	auto format(const siddiqsoft::RESTResponseType& sv, std::format_context& ctx)
	{
		return std::formatter<std::string>::format(sv.encode(), ctx);
	}
};
#endif // !RESTREQUESTTYPE_HPP
