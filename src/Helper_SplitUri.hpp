#pragma once
#ifndef SPLITURI_HPP
#define SPLITURI_HPP

#include <tuple>
#include <string>
#include <type_traits>
#include "nlohmann/json.hpp"
#include "siddiqsoft/string2map.hpp"
#include "siddiqsoft/string2vector.hpp"


namespace siddiqsoft
{
	/*
		https://en.wikipedia.org/wiki/Uniform_Resource_Identifier#Syntax

			  userinfo       host      port
			  ┌──┴───┐ ┌──────┴──────┐ ┌┴┐
	  https://john.doe@www.example.com:123/forum/questions/?tag=networking&order=newest#top
	  └─┬─┘   └───────────┬──────────────┘└───────┬───────┘ └───────────┬─────────────┘ └┬┘
	  scheme          authority                  path                 query           fragment

	  ldap://[2001:db8::7]/c=GB?objectClass?one
	  └┬─┘   └─────┬─────┘└─┬─┘ └──────┬──────┘
	  scheme   authority   path      query

	  mailto:John.Doe@example.com
	  └─┬──┘ └────┬─────────────┘
	  scheme     path

	  news:comp.infosystems.www.servers.unix
	  └┬─┘ └─────────────┬─────────────────┘
	  scheme            path

	  tel:+1-816-555-1212
	  └┬┘ └──────┬──────┘
	  scheme    path

	  telnet://192.0.2.16:80/
	  └─┬──┘   └─────┬─────┘│
	  scheme     authority  path

	  urn:oasis:names:specification:docbook:dtd:xml:4.1.2
	  └┬┘ └──────────────────────┬──────────────────────┘
	  scheme                    path
	*/

	template <class T> struct Authority
	{
		T        userInfo {};
		T        seperatorAt {"@"};
		T        host {};
		T        separatorColon {":"};
		uint16_t port {0};

		operator std::string()
		{
			return std::format("{}{}{}{}{}",
			                   userInfo,
			                   !userInfo.empty() ? seperatorAt : T {},
			                   host,
			                   port > 0 ? separatorColon : T {},
			                   port > 0 ? port : T {});
		}
	};

	enum class UriScheme
	{
		WebHttp,
		WebHttps,
		Ldap,
		Mailto,
		News,
		Tel,
		Telnet,
		Urn,
		Unknown
	};


	NLOHMANN_JSON_SERIALIZE_ENUM(UriScheme,
	                             {{UriScheme::WebHttp, "http"},
	                              {UriScheme::WebHttps, "https"},
	                              {UriScheme::Ldap, "ldap"},
	                              {UriScheme::Mailto, "mailto"},
	                              {UriScheme::News, "news"},
	                              {UriScheme::Tel, "tel"},
	                              {UriScheme::Telnet, "telnet"},
	                              {UriScheme::Urn, "urn"},
	                              {UriScheme::Unknown, nullptr}});


	template <class T> struct Uri
	{
		UriScheme                          scheme;
		T                                  separatorColonSlashSlash {"://"};
		Authority<T>                       authority;
		T                                  url {}; // contains the "balance" post Authority section
		T                                  separatorSlash {"/"};
		std::vector<T>                     path {};
		T                                  separatorQuestion {"?"};
		std::map<std::string, std::string> query {};
		std::string                        fragment {};
	};


	template <class T> Uri<T> static SplitUri(const T& aEndpoint)
	{
		Uri<T> uri {};

#pragma region Comparision Helpers
		constexpr auto matchHttps = []() -> auto
		{
			if constexpr (std::is_same_v<T, std::wstring>) return L"https://";
			return "https://";
		}
		();
		constexpr auto matchHttp = []() -> auto
		{
			if constexpr (std::is_same_v<T, std::wstring>) return L"http://";
			return "http://";
		}
		();
		constexpr auto matchSlash = []() -> auto
		{
			if constexpr (std::is_same_v<T, std::wstring>) return L"/";
			return "/";
		}
		();
		constexpr auto matchColon = []() -> auto
		{
			if constexpr (std::is_same_v<T, std::wstring>) return L":";
			return ":";
		}
		();
		constexpr auto matchHash = []() -> auto
		{
			if constexpr (std::is_same_v<T, std::wstring>) return L"#";
			return "#";
		}
		();
		constexpr auto matchAt = []() -> auto
		{
			if constexpr (std::is_same_v<T, std::wstring>) return L"@";
			return "@";
		}
		();
		constexpr auto matchQuestion = []() -> auto
		{
			if constexpr (std::is_same_v<T, std::wstring>) return L"?";
			return "?";
		}
		();
		constexpr auto matchEq = []() -> auto
		{
			if constexpr (std::is_same_v<T, std::wstring>) return L"=";
			return "=";
		}
		();
		constexpr auto matchAmp = []() -> auto
		{
			if constexpr (std::is_same_v<T, std::wstring>) return L"&";
			return "&";
		}
		();


#pragma endregion

		size_t posProtocol = std::string::npos;

		if (aEndpoint.starts_with(matchHttps))
		{
			posProtocol = 8;
			uri.scheme  = UriScheme::WebHttps;
		}
		else if (aEndpoint.starts_with(matchHttp))
		{
			posProtocol = 7;
			uri.scheme  = UriScheme::WebHttp;
		}

		if (posProtocol != std::string::npos)
		{
			if (auto posAt = aEndpoint.find(matchAt, posProtocol); posAt != std::string::npos)
			{
				// We have a user part
				uri.authority.userInfo = aEndpoint.substr(posProtocol, (posAt - posProtocol));
				// Advance to the end of the user "@" which begins the host part
				posProtocol = ++posAt;
			}

			// Search for the next `/` which would allow us to extract the server:port portion.
			// It is possible that the aEndpoint does not contain the trailing `/` in which case
			// the url portion is "empty" and the only thing left is to attempt to extract the server
			// portion.
			auto pos2 = aEndpoint.find(matchSlash, posProtocol);

			// Extract the server:port portion.. make sure we don't calculate based on missing `/`
			uri.authority.port = 80; // replace later if present
			uri.authority.host =
					aEndpoint.substr(posProtocol, pos2 != std::string::npos ? (pos2 - posProtocol) : std::string::npos);
			if (auto pos3 = uri.authority.host.find(matchColon); pos3 != std::string::npos)
			{
				// First Extract the port
				uri.authority.port = std::stoi(uri.authority.host.substr(++pos3));
				// Reset the server
				uri.authority.host = uri.authority.host.substr(0, (--pos3 - 0));
			}

			if (pos2 != std::string::npos)
			{
				// Finally, the rest of the endpoint is the url portion.
				uri.url = aEndpoint.substr(pos2);

				// Continue to split the path, query and fragment
				// Pull out the fragment
				auto posFragment = aEndpoint.rfind(matchHash);
				if (posFragment != std::string::npos)
				{
					// We have a fragment
					uri.fragment = aEndpoint.substr(posFragment+1);
				}

				auto posQueryPart = aEndpoint.find(matchQuestion, pos2);

				auto pathSegment =
						aEndpoint.substr(pos2, posQueryPart != std::string::npos ? posQueryPart - (pos2 + 1) : std::string::npos);
				if (!pathSegment.empty()) uri.path = siddiqsoft::string2vector::parse<T>(pathSegment, matchSlash);

				auto querySegment = aEndpoint.substr(posQueryPart+1,
				                                     posFragment != std::string::npos ? (posFragment) - (posQueryPart + 1)
				                                                                      : std::string::npos);
				if (!querySegment.empty()) uri.query = siddiqsoft::string2map::parse<T>(querySegment, matchEq, matchAmp);
			}
		}

		return uri;
	}

} // namespace siddiqsoft

#endif // !SPLITURI_HPP
