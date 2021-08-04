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
	SERVICES; LOSS OF USE, data_, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
	OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#ifndef RESTCL_HPP
#define RESTCL_HPP

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
	enum class RESTMethodType
	{
		Get,
		Patch,
		Post,
		Put,
		Delete,
		Info
	};


	/// @brief A REST request utility class
	template <RESTMethodType ReqType> class SimpleRestRequestType
	{
	public:
		/// @brief Constructor with endpoint and optional headers and json content
		/// @param endpoint Fully qualified http schema uri
		/// @param h Optional json containing the headers
		/// @param c Optional json of the content
		SimpleRestRequestType(const std::string& endpoint, const nlohmann::json& h = {}, const nlohmann::json& c = {})
		{
			uri_ = SplitUri<>(endpoint);

			if (h.is_object()) { data_["headers"] = h; }
			if (c.is_object()) { data_["content"] = c; }

			data_["request"]["uri"]            = uri_.url;
			data_["headers"]["Host"]           = std::format("{}:{}", uri_.authority.host, uri_.authority.port);
			data_["headers"]["Content-Length"] = 0;
			data_["headers"]["User-Agent"]     = "siddiqsoft/restcl";
		}

		/// @brief Set the Uri via the endpoint
		/// @param e Fully qualified endpoint uri with http scheme
		/// @return Self
		auto endpoint(const std::string& e)
		{
			uri_ = SplitUri(endpoint);
			return *this;
		}

		/// @brief Returns the mutable reference to the headers json
		/// @return Json reference to headers element
		nlohmann::json& headers() { return data_.at("headers"); }

		/// @brief Returns the mutable reference to the request json
		/// @return Json reference to request element
		nlohmann::json& request() { return data_.at("request"); }

		/// @brief Return the parsed endpoint into Uri
		/// @return Uri structure
		auto uri() const { return uri_; }

		/// @brief Set the content (non-JSON)
		/// @param ctype Content-Type
		/// @param c The content
		/// @return Self
		SimpleRestRequestType<ReqType>& content(const std::string& ctype, const std::string& c)
		{
			data_["headers"]["Content-Type"]   = ctype;
			data_["headers"]["Content-Length"] = c.length();
			data_["content"]                   = c;
			return *this;
		}

		/// @brief Set the content to json
		/// @param c JSON content
		/// @return Self
		SimpleRestRequestType<ReqType>& content(const nlohmann::json& c)
		{
			data_["content"]                   = c;
			data_["headers"]["Content-Length"] = c.dump().length();
			data_["headers"]["Content-Type"]   = "application/json";
			return *this;
		}

		/// @brief Encode the headers to the given argument
		/// @param rs String where the headers is "written-to".
		void encodeHeaders_to(std::string& rs) const
		{
			for (auto& [k, v] : data_["headers"].items())
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
			auto&       hs = data_.at("headers");
			auto&       rl = data_.at("request");

			// Request Line
			std::format_to(std::back_inserter(rs),
			               "{} {} {}\r\n",
			               rl["method"].get<std::string>(),
			               rl["uri"].get<std::string>(),
			               rl["version"].get<std::string>());

			// Build the content to ensure we have the content-type
			if (data_["content"].is_object()) { body = data_["content"].dump(); }
			else if (data_["content"].is_string())
			{
				body = data_["content"].get<std::string>();
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
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimpleRestRequestType<ReqType>, data_);

	protected:
		Uri<std::string> uri_;
		nlohmann::json   data_ {{"request", {{"method", ReqType}, {"uri", nullptr}, {"version", "HTTP/1.1"}}},
                              {"headers", nullptr},
                              {"content", nullptr}};
	};


	NLOHMANN_JSON_SERIALIZE_ENUM(RESTMethodType,
	                             {{RESTMethodType::Get, "GET"},
	                              {RESTMethodType::Patch, "PATCH"},
	                              {RESTMethodType::Put, "PUT"},
	                              {RESTMethodType::Post, "POST"},
	                              {RESTMethodType::Delete, "DELETE"},
	                              {RESTMethodType::Info, "INFO"}});

} // namespace siddiqsoft


template <> struct std::formatter<siddiqsoft::RESTMethodType> : std::formatter<std::string>
{
	auto format(const siddiqsoft::RESTMethodType& sv, std::format_context& ctx)
	{
		return std::formatter<std::string>::format(nlohmann::json(sv).get<std::string>(), ctx);
	}
};


template <siddiqsoft::RESTMethodType R> struct std::formatter<siddiqsoft::SimpleRestRequestType<R>> : std::formatter<std::string>
{
	auto format(const siddiqsoft::SimpleRestRequestType<R>& sv, std::format_context& ctx)
	{
		return std::formatter<std::string>::format(sv.encode(), ctx);
	}
};

#endif // !RESTCL_HPP
