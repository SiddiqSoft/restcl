#pragma once
#ifndef HTTPREQUESTTYPE_HPP
#define HTTPREQUESTTYPE_HPP

#include <chrono>
#include <string>
#include <functional>
#include <memory>
#include <format>

#include "nlohmann/json.hpp"


#include "RESTMethodType.hpp"
#include "HttpRequestLineType.hpp"


namespace siddiqsoft
{
	class HttpRequestType
	{
		nlohmann::json data {{"request", HttpRequestLineType {}},
		                     {"headers",
		                      {{"Host", nullptr},
		                       {"User-Agent", "siddiqsoft/restcl"},
		                       {"Content-Type", "application/json"},
		                       {"Content-Type", 0}}},
		                     {"content", nullptr}};

	public:
		HttpRequestType() { }

		HttpRequestType(const HttpRequestLineType& arg)
			: data({{"request", arg},
		            {"headers",
		             {{"Host", nullptr},
		              {"User-Agent", "siddiqsoft/restcl"},
		              {"Content-Type", "application/json"},
		              {"Content-Type", 0}}},
		            {"content", nullptr}})
		{
		}

		HttpRequestType(const nlohmann::json& src)
			: data(src)
		{
		}

		HttpRequestType(nlohmann::json&& src) { std::swap(data, src); }

		nlohmann::json& request() { return data.at("request"); }
		nlohmann::json& headers() { return data.at("headers"); }
		nlohmann::json& content() { return data.at("content"); }

		operator bool()
		{
			// Validate
			return data.contains("request") && data.contains("headers");
		}

		operator std::string()
		{
			std::string rs;
			std::string body;

			// Request Line
			std::format_to(std::back_inserter(rs),
			               "{} {} {}\r\n",
			               request().at("method").get<std::string>(),
			               request().at("path").get<std::string>(),
			               request().at("version").get<std::string>());

			// Build the content to ensure we have the content-type
			if (!content().empty())
			{
				body                        = content().dump();
				headers()["Content-Length"] = body.length();
			}
			else
			{
				headers()["Content-Length"] = 0;
			}

			for (auto h = headers().begin(); h != headers().end(); ++h)
			{
				std::format_to(std::back_inserter(rs), "{}: {}\r\n", h.key(), h.value().dump());
			}

			std::format_to(std::back_inserter(rs), "\r\n");
			if (!body.empty()) { std::format_to(std::back_inserter(rs), "{}", body); }

			return std::move(rs);
		}

		friend void to_json(nlohmann::json& dest, const HttpRequestType& src);
		friend void from_json(const nlohmann::json& src, HttpRequestType& dest);
	};


	void to_json(nlohmann::json& dest, const HttpRequestType& src) { dest = src.data; }

	//void from_json(const nlohmann::json& src, HttpRequestType& dest)
	//{
	//	src.at("request").get_to(dest.requestLine);
	//	src.at("headers").get_to(dest.data.at("headers"));
	//	if (src.contains("content")) { src.at("content").get_to(dest.data.at("content")); }
	//}
} // namespace siddiqsoft

#endif // !HTTPREQUESTTYPE_HPP
