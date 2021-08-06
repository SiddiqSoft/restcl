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
		Head,
		Options
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(RESTMethodType,
	                             {{RESTMethodType::Get, "GET"},
	                              {RESTMethodType::Patch, "PATCH"},
	                              {RESTMethodType::Put, "PUT"},
	                              {RESTMethodType::Post, "POST"},
	                              {RESTMethodType::Delete, "DELETE"},
	                              {RESTMethodType::Head, "HEAD"},
	                              {RESTMethodType::Options, "OPTIONS"}});

	enum class HTTPProtocolVersion
	{
		Http2,
		Http11,
		Http10
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(HTTPProtocolVersion,
	                             {{HTTPProtocolVersion::Http2, "HTTP/2"},
	                              {HTTPProtocolVersion::Http11, "HTTP/1.1"},
	                              {HTTPProtocolVersion::Http10, "HTTP/1.0"}});


	/// @brief A REST request utility class. Models the request a JSON document with `request`, `headers` and `content` elements.
	template <HTTPProtocolVersion HttpVer = HTTPProtocolVersion::Http11> class RESTRequestType
	{
	public:
		/// @brief Constructor with endpoint and optional headers and json content
		/// @param endpoint Fully qualified http schema uri
		/// @param h Optional json containing the headers
		RESTRequestType(const RESTMethodType& reqMethod, const std::string& endpoint, const nlohmann::json& h = nullptr)
		{
			uri = SplitUri(endpoint);

			if (h.is_object() && !h.is_null()) { rrd["headers"] = h; }

			rrd["request"]["uri"]    = uri.urlPart;
			rrd["request"]["method"] = reqMethod;

			// Enforce some default headers
			auto& hs = rrd.at("headers");
			if (!hs.contains("Date")) hs["Date"] = Date_rfc1123();
			if (!hs.contains("Accept")) hs["Accept"] = "application/json";
			if (!hs.contains("Host")) hs["Host"] = std::format("{}:{}", uri.authority.host, uri.authority.port);
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
			if (c != nullptr)
			{
				if (h.is_null() || (h.is_object() && !h.contains("Content-Type"))) rrd["headers"]["Content-Type"] = "text/plain";
				rrd["headers"]["Content-Length"] = strlen(c);
				rrd["content"]                   = c;
			}
			else
			{
				rrd["headers"]["Content-Length"] = 0;
			}
		}


		/// @brief Set the Uri via the endpoint
		/// @param e Fully qualified endpoint uri with http scheme
		/// @return Self
		auto& setEndpoint(const std::string& e)
		{
			uri = SplitUri(e);
			return *this;
		}


		/// @brief Access the "headers", "request", "content" in the json object
		/// @param key Allows access into the json object via string or json_pointer
		/// @return Non-mutable reference to the specified element.
		const auto& operator[](const auto& key) const { return rrd.at(key); }


		/// @brief Set the content (non-JSON)
		/// @param ctype Content-Type
		/// @param c The content
		/// @return Self
		RESTRequestType& setContent(const std::string& ctype, const std::string& c)
		{
			if (!c.empty())
			{
				rrd["headers"]["Content-Type"]   = ctype;
				rrd["headers"]["Content-Length"] = c.length();
				rrd["content"]                   = c;
			}
			return *this;
		}

		/// @brief Set the content to json
		/// @param c JSON content
		/// @return Self
		RESTRequestType& setContent(const nlohmann::json& c)
		{
			if (!c.is_null())
			{
				rrd["content"]                   = c;
				rrd["headers"]["Content-Length"] = c.dump().length();
				rrd["headers"]["Content-Type"]   = "application/json";
			}
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
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(RESTRequestType, uri, rrd);
		friend std::ostream& operator<<(std::ostream&, const RESTRequestType<>&);

	public:
		Uri<std::string> uri;

	protected:
		nlohmann::json rrd {{"request", {{"method", RESTMethodType::Get}, {"uri", nullptr}, {"version", HttpVer}}},
		                    {"headers", nullptr},
		                    {"content", nullptr}};
	};


	static std::ostream& operator<<(std::ostream& os, const RESTRequestType<>& src)
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

		/// @brief Access the "headers", "request", "content" in the json object
		/// @param key Allows access into the json object via string or json_pointer
		/// @return Non-mutable reference to the specified element.
		const auto& operator[](const auto& key) const { return rrd.at(key); }


		/// @brief Mutable access to the underlying json object
		/// @param key Allows access into the json object via string or json_pointer
		/// @return Mutable reference to the specified element.
		auto& operator[](const auto& key) { return rrd.at(key); }


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

		/// @brief Returns the IO error if present otherwise returns the HTTP status code and reason
		/// @return tuple with the ioError/Error or StatusCode/Reason
		std::tuple<uint32_t, std::string> getError() const
		{
			return {ioErrorCode > 0 ? ioErrorCode : statusCode, ioErrorCode > 0 ? ioError : rrd["response"].value("reason", "")};
		}

	public:
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(RESTResponseType, statusCode, ioErrorCode, ioError, rrd);
		friend std::ostream& operator<<(std::ostream&, const RESTResponseType&);

	protected:
		uint16_t       statusCode {0};
		uint32_t       ioErrorCode {0};
		std::string    ioError {};
		nlohmann::json rrd {{"response", {{"status", statusCode}, {"reason", ""}, {"version", HTTPProtocolVersion::Http11}}},
		                    {"headers", nullptr},
		                    {"content", nullptr}};
	};


	static std::ostream& operator<<(std::ostream& os, const RESTResponseType& src)
	{
		std::format_to(std::ostream_iterator<char>(os), "{}", src);
		return os;
	}
} // namespace siddiqsoft


template <> struct std::formatter<siddiqsoft::HTTPProtocolVersion> : std::formatter<std::string>
{
	auto format(const siddiqsoft::HTTPProtocolVersion& sv, std::format_context& ctx)
	{
		return std::formatter<std::string>::format(nlohmann::json(sv).get<std::string>(), ctx);
	}
};


/// @brief Formatters for the RESTMethodType
template <> struct std::formatter<siddiqsoft::RESTMethodType> : std::formatter<std::string>
{
	auto format(const siddiqsoft::RESTMethodType& sv, std::format_context& ctx)
	{
		return std::formatter<std::string>::format(nlohmann::json(sv).get<std::string>(), ctx);
	}
};


/// @brief Formatter for RESTRequestType
template <> struct std::formatter<siddiqsoft::RESTRequestType<>> : std::formatter<std::string>
{
	auto format(const siddiqsoft::RESTRequestType<>& sv, std::format_context& ctx)
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
