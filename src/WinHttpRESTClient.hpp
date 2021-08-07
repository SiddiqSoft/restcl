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
	SERVICES; LOSS OF USE, d_, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
	OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#ifndef RESTCLWINHTTP_HPP
#define RESTCLWINHTTP_HPP

#include <chrono>
#include <string>
#include <functional>
#include <memory>
#include <format>


#include <windows.h>

#if !_WINHTTPX_
#pragma message("--- " __FILE__ "--- Doing include winhttp..")
#include <winhttp.h>
#else
#pragma message("--- " __FILE__ "--- Skip include winhttp..")
#endif

#pragma comment(lib, "winhttp")


#include "nlohmann/json.hpp"
#include "RESTClient.hpp"
#include "siddiqsoft/acw32h.hpp"


namespace siddiqsoft
{
#pragma region WinInet error code map
	static std::map<uint32_t, std::string_view> WinInetErrorCodes {
			{12001, std::string_view("ERROR_INTERNET_OUT_OF_HANDLES: No more handles could be generated at this time.")},
			{12002, std::string_view("ERROR_INTERNET_TIMEOUT: The request has timed out.")},
			{12003,
	         std::string_view("ERROR_INTERNET_EXTENDED_ERROR: An extended error was returned from the server. This is typically a "
	                          "string or buffer "
	                          "containing a verbose error message. Call InternetGetLastResponseInfo to retrieve the error text.")},
			{12004, std::string_view("ERROR_INTERNET_INTERNAL_ERROR: An internal error has occurred.")},
			{12005, std::string_view("ERROR_INTERNET_INVALID_URL:The URL is invalid.")},
			{12006,
	         std::string_view("ERROR_INTERNET_UNRECOGNIZED_SCHEME: The URL scheme could not be recognized or is not supported.")},
			{12007, std::string_view("ERROR_INTERNET_NAME_NOT_RESOLVED: The server name could not be resolved.")},
			{12008, std::string_view("ERROR_INTERNET_PROTOCOL_NOT_FOUND: The requested protocol could not be located.")},
			{12009,
	         std::string_view("ERROR_INTERNET_INVALID_OPTION: A request to InternetQueryOption or InternetSetOption specified an "
	                          "invalid option "
	                          "value.")},
			{12010,
	         std::string_view("ERROR_INTERNET_BAD_OPTION_LENGTH: The length of an option supplied to InternetQueryOption or "
	                          "InternetSetOption is "
	                          "incorrect for the type of option specified.")},
			{12011, std::string_view("ERROR_INTERNET_OPTION_NOT_SETTABLE: The request option cannot be set, only queried.")},
			{12012,
	         std::string_view("ERROR_INTERNET_SHUTDOWN: The Win32 Internet function support is being shut down or unloaded.")},
			{12013,
	         std::string_view("ERROR_INTERNET_INCORRECT_USER_NAME: The request to connect and log on to an FTP server could not be "
	                          "completed "
	                          "because the supplied user name is incorrect.")},
			{12014,
	         std::string_view("ERROR_INTERNET_INCORRECT_PASSWORD: The request to connect and log on to an FTP server could not be "
	                          "completed because "
	                          "the supplied password is incorrect.")},
			{12015,
	         std::string_view("ERROR_INTERNET_LOGIN_FAILURE: The request to connect to and log on to an FTP server failed.")},
			{12016, std::string_view("ERROR_INTERNET_INVALID_OPERATION: The requested operation is invalid.")},
			{12017,
	         std::string_view("ERROR_INTERNET_OPERATION_CANCELLED: The operation was canceled, usually because the handle on which "
	                          "the request was "
	                          "operating was closed before the operation completed.")},
			{12018,
	         std::string_view(
					 "ERROR_INTERNET_INCORRECT_HANDLE_TYPE: The type of handle supplied is incorrect for this operation.")},
			{12019,
	         std::string_view("ERROR_INTERNET_INCORRECT_HANDLE_STATE: The requested operation cannot be carried out because the "
	                          "handle supplied is "
	                          "not in the correct state.")},
			{12020, std::string_view("ERROR_INTERNET_NOT_PROXY_REQUEST: The request cannot be made via a proxy.")},
			{12021, std::string_view("ERROR_INTERNET_REGISTRY_VALUE_NOT_FOUND: A required registry value could not be located.")},
			{12022,
	         std::string_view("ERROR_INTERNET_BAD_REGISTRY_PARAMETER: A required registry value was located but is an incorrect "
	                          "type or has an "
	                          "invalid value.")},
			{12023, std::string_view("ERROR_INTERNET_NO_DIRECT_ACCESS: Direct network access cannot be made at this time.")},
			{12024,
	         std::string_view("ERROR_INTERNET_NO_CONTEXT: An asynchronous request could not be made because a zero context value "
	                          "was supplied.")},
			{12025,
	         std::string_view("ERROR_INTERNET_NO_CALLBACK: An asynchronous request could not be made because a callback function "
	                          "has not been set.")},
			{12026,
	         std::string_view("ERROR_INTERNET_REQUEST_PENDING: The required operation could not be completed because one or more "
	                          "requests are "
	                          "pending.")},
			{12027, std::string_view("ERROR_INTERNET_INCORRECT_FORMAT: The format of the request is invalid.")},
			{12028, std::string_view("ERROR_INTERNET_ITEM_NOT_FOUND: The requested item could not be located.")},
			{12029, std::string_view("ERROR_INTERNET_CANNOT_CONNECT: The attempt to connect to the server failed.")},
			{12030, std::string_view("ERROR_INTERNET_CONNECTION_ABORTED: The connection with the server has been terminated.")},
			{12031, std::string_view("ERROR_INTERNET_CONNECTION_RESET: The connection with the server has been reset.")},
			{12032, std::string_view("ERROR_INTERNET_FORCE_RETRY: Calls for the Win32 Internet function to redo the request.")},
			{12033, std::string_view("ERROR_INTERNET_INVALID_PROXY_REQUEST: The request to the proxy was invalid.")},
			{12036, std::string_view("ERROR_INTERNET_HANDLE_EXISTS: The request failed because the handle already exists.")},
			{12037,
	         std::string_view("ERROR_INTERNET_SEC_CERT_DATE_INVALID: SSL certificate date that was received from the server is "
	                          "bad. The certificate "
	                          "is expired.")},
			{12038,
	         std::string_view("ERROR_INTERNET_SEC_CERT_CN_INVALID: SSL certificate common name (host name field) is incorrect. For "
	                          "example, if you "
	                          "entered www.server.com and the common name on the certificate says www.different.com.")},
			{12039,
	         std::string_view("ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR: The application is moving from a non-SSL to an SSL "
	                          "connection because of a "
	                          "redirect.")},
			{12040,
	         std::string_view("ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR: The application is moving from an SSL to an non-SSL "
	                          "connection because of a "
	                          "redirect.")},
			{12041,
	         std::string_view("ERROR_INTERNET_MIXED_SECURITY: Indicates that the content is not entirely secure. Some of the "
	                          "content being viewed "
	                          "may have come from unsecured servers.")},
			{12042,
	         std::string_view("ERROR_INTERNET_CHG_POST_IS_NON_SECURE: The application is posting and attempting to change multiple "
	                          "lines of text on "
	                          "a server that is not secure.")},
			{12043,
	         std::string_view(
					 "ERROR_INTERNET_POST_IS_NON_SECURE: The application is posting data to a server that is not secure.")},
			{12044, std::string_view("ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED: Client certificate is needed.")},
			{12110,
	         std::string_view(
					 "ERROR_FTP_TRANSFER_IN_PROGRESS: The requested operation cannot be made on the FTP session handle because an "
					 "operation is already in progress.")},
			{12111, std::string_view("ERROR_FTP_DROPPED: The FTP operation was not completed because the session was aborted.")},
			{12130,
	         std::string_view(
					 "ERROR_GOPHER_PROTOCOL_ERROR: An error was detected while parsing data returned from the gopher server.")},
			{12131, std::string_view("ERROR_GOPHER_NOT_FILE: The request must be made for a file locator.")},
			{12132,
	         std::string_view("ERROR_GOPHER_DATA_ERROR: An error was detected while receiving data from the gopher server.")},
			{12133, std::string_view("ERROR_GOPHER_END_OF_DATA: The end of the data has been reached.")},
			{12134, std::string_view("ERROR_GOPHER_INVALID_LOCATOR: The supplied locator is not valid.")},
			{12135,
	         std::string_view("ERROR_GOPHER_INCORRECT_LOCATOR_TYPE: The type of the locator is not correct for this operation.")},
			{12136,
	         std::string_view("ERROR_GOPHER_NOT_GOPHER_PLUS: The requested operation can only be made against a Gopher+ server or "
	                          "with a locator "
	                          "that specifies a Gopher+ operation.")},
			{12137, std::string_view("ERROR_GOPHER_ATTRIBUTE_NOT_FOUND: The requested attribute could not be located.")},
			{12138, std::string_view("ERROR_GOPHER_UNKNOWN_LOCATOR: The locator type is unknown.")},
			{12150, std::string_view("ERROR_HTTP_HEADER_NOT_FOUND: The requested header could not be located.")},
			{12151, std::string_view("ERROR_HTTP_DOWNLEVEL_SERVER: The server did not return any headers.")},
			{12152, std::string_view("ERROR_HTTP_INVALID_SERVER_RESPONSE: The server response could not be parsed.")},
			{12153, std::string_view("ERROR_HTTP_INVALID_HEADER: The supplied header is invalid.")},
			{12154, std::string_view("ERROR_HTTP_INVALID_QUERY_REQUEST: The request made to HttpQueryInfo is invalid.")},
			{12155, std::string_view("ERROR_HTTP_HEADER_ALREADY_EXISTS: The header could not be added because it already exists.")},
			{12156,
	         std::string_view("ERROR_HTTP_REDIRECT_FAILED: The redirection failed because either the scheme changed (for example, "
	                          "HTTP to FTP) or "
	                          "all attempts made to redirect failed (default is five attempts). ")},
			{12175, std::string_view("ERROR_WINHTTP_SECURE_FAILURE: Port specification might be invalid.")}};

	static std::string messageFromWininetCode(uint32_t errCode)
	{
		return WinInetErrorCodes.find(errCode) != WinInetErrorCodes.end()
					   ? std::format("{}-{}", errCode, WinInetErrorCodes[errCode])
					   : std::to_string(errCode);
	}
#pragma endregion

	constexpr size_t      HEADER_SERIALIZE_BUFFER {4096};
	constexpr DWORD       MAXBUFSIZE {8192};
	constexpr DWORD       HTTPS_MAXRETRY {7};
	static const char*    RESTCL_ACCEPT_TYPES[]   = {"application/json", "text/json", "*/*", NULL};
	static const wchar_t* RESTCL_ACCEPT_TYPES_W[] = {L"application/json", L"text/json", L"*/*", NULL};


	/// @brief Windows implementation of the RESTClient
	class WinHttpRESTClient : public RESTClient
	{
		WinHttpRESTClient(const WinHttpRESTClient&) = delete;
		WinHttpRESTClient& operator=(const WinHttpRESTClient&) = delete;

	private:
		/// @brief "Upgrades" a string to wstring by copying the chars without any actual conversion.
		/// Only use this for where there is a need to provide wstring and the underlying elements are ASCII.
		/// @param src ASCII string. DO NOT USE utf-8
		/// @return Upgraded wstring
		auto asciiToWstringUpgrade(const std::string& src)
		{
			std::wstring rawAsciiToWstring {src.begin(), src.end()};
			return std::move(rawAsciiToWstring);
		}

	public:
		WinHttpRESTClient() { }


		/// @brief Implements a synchronous send of the request
		/// @param req Request object
		/// @param callback Required callback
		void send(const RESTRequestType<>& req, std::function<void(const RESTRequestType<>&, const RESTResponseType&)>&& callback)
		{
			HRESULT  hr {E_FAIL};
			DWORD    dwBytesRead {0}, dwError {0};
			uint32_t nRetry {0}, nError {0};
			char     cBuf[MAXBUFSIZE] {};
			DWORD    dwFlagsSize  = 0; //sizeof(SECURE_FLAGS);
			auto     strUserAgent = req["headers"].value("User-Agent", "");


			if (ACW32HINTERNET hSession {
						WinHttpOpen(asciiToWstringUpgrade(strUserAgent).c_str(), WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0)};
			    hSession != NULL)
			{
				auto& strServer = req.uri.authority.host;
				if (ACW32HINTERNET hConnect {
							WinHttpConnect(hSession, asciiToWstringUpgrade(strServer).c_str(), req.uri.authority.port, 0)};
				    hConnect != NULL)
				{
					auto strMethod  = req["request"].value("method", "");
					auto strUrl     = req.uri.urlPart;
					auto strVersion = req["request"].value("version", "");

					if (ACW32HINTERNET hRequest {WinHttpOpenRequest(hConnect,
					                                                asciiToWstringUpgrade(strMethod).c_str(),
					                                                asciiToWstringUpgrade(strUrl).c_str(),
					                                                asciiToWstringUpgrade(strVersion).c_str(),
					                                                NULL,
					                                                RESTCL_ACCEPT_TYPES_W,
					                                                (req.uri.scheme == UriScheme::WebHttps)
					                                                        ? WINHTTP_FLAG_SECURE | WINHTTP_FLAG_REFRESH
					                                                        : WINHTTP_FLAG_REFRESH)};
					    hRequest != NULL)
					{
						auto        contentLength = req["headers"].value("Content-Length", 0);
						std::string strHeaders;
						req.encodeHeaders_to(strHeaders);
						std::wstring requestHeaders = asciiToWstringUpgrade(strHeaders);
						nError                      = WinHttpAddRequestHeaders(
                                hRequest, requestHeaders.c_str(), requestHeaders.length(), WINHTTP_ADDREQ_FLAG_ADD);

						// Set this option so we don't incur a roundtrip delay
						//ll	   = __LINE__;
						//nError = WinHttpSetOption(
						//		hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);

						dwError = ERROR_SUCCESS;

						nError = WinHttpSendRequest(hRequest,
						                            WINHTTP_NO_ADDITIONAL_HEADERS,
						                            0,
						                            contentLength > 0 ? LPVOID(req.getContent().c_str()) : NULL,
						                            contentLength,
						                            contentLength,
						                            NULL);

						if (nError == FALSE, dwError = GetLastError(); dwError == ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED)
						{
							nError = WinHttpSetOption(
									hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);
							if (nError == TRUE)
							{
								nError = WinHttpSendRequest(hRequest,
								                            WINHTTP_NO_ADDITIONAL_HEADERS,
								                            0,
								                            contentLength > 0 ? LPVOID(req.getContent().c_str()) : NULL,
								                            contentLength,
								                            contentLength,
								                            NULL);
							}
						}


						// Receive phase
						RESTResponseType resp;

						// Get the "response" and the headers..
						if (nError == FALSE) { dwError = GetLastError(); }
						else
						{
							// Signals we should finish the request and wait for response from server.
							nError = WinHttpReceiveResponse(hRequest, NULL);
							if (nError == FALSE) { dwError = GetLastError(); }
							else
							{
								// Get the HTTP respose status
								std::wstring buff {};
								DWORD        szBuff    = 256;
								DWORD        nextIndex = 0;

								buff.resize(szBuff);
								if (TRUE == WinHttpQueryHeaders(hRequest,
								                                WINHTTP_QUERY_STATUS_CODE,
								                                WINHTTP_HEADER_NAME_BY_INDEX,
								                                buff.data(),
								                                &szBuff,
								                                &nextIndex))
								{
									buff.resize(szBuff);
									if (!buff.empty()) resp["response"]["status"] = std::stoi(buff);
								}

								// Get the reason phrase
								szBuff = 256;
								buff.resize(szBuff);
								buff.clear();
								if (TRUE == WinHttpQueryHeaders(hRequest,
								                                WINHTTP_QUERY_STATUS_TEXT,
								                                WINHTTP_HEADER_NAME_BY_INDEX,
								                                buff.data(),
								                                &szBuff,
								                                &nextIndex))
								{
									// Convert from wstring to string
									thread_local std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

									//buff.resize(szBuff);
									if (!buff.empty()) resp["response"]["reason"] = converter.to_bytes(buff);
								}

								// Get the headers next..
								DWORD dwSize = 0;

								// First, use WinHttpQueryHeaders to obtain the size of the buffer.
								WinHttpQueryHeaders(hRequest,
								                    WINHTTP_QUERY_RAW_HEADERS_CRLF,
								                    WINHTTP_HEADER_NAME_BY_INDEX,
								                    NULL,
								                    &dwSize,
								                    WINHTTP_NO_HEADER_INDEX);

								// Allocate memory for the buffer.
								if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
								{
									auto lpOutBuffer = std::make_unique<wchar_t[]>(dwSize);

									// Now, use WinHttpQueryHeaders to retrieve the header.
									nError = WinHttpQueryHeaders(hRequest,
									                             WINHTTP_QUERY_RAW_HEADERS_CRLF,
									                             WINHTTP_HEADER_NAME_BY_INDEX,
									                             lpOutBuffer.get(),
									                             &dwSize,
									                             WINHTTP_NO_HEADER_INDEX);
									if (nError == TRUE && (lpOutBuffer != nullptr))
									{
										// We need to skip the first line of the respose as it is the status response line.
										// Decode the CRLF string into key-value elements.
										std::wstring src(static_cast<const wchar_t*>(lpOutBuffer.get()), dwSize);
										auto         startOfHeaders = src.find(L"\r\n");
										src.erase(0, startOfHeaders + 2);
										resp["headers"] =
												string2map::parse<std::wstring, std::string, std::map<std::string, std::string>>(
														src, L": ", L"\r\n");
									}
								}
							}
						}

						// Next stage is to check for any errors and if none, get the body
						if (dwError == ERROR_WINHTTP_NAME_NOT_RESOLVED)
						{
							callback(req, {{dwError, messageFromWininetCode(dwError)}});
							//throw(invalid_argument(
							//		std::format("HttpSendRequest() Failed Invalid host; dwError={}", messageFromWininetCode(dwError))));
						}
						else if ((dwError == ERROR_WINHTTP_CANNOT_CONNECT) || (dwError == ERROR_WINHTTP_CONNECTION_ERROR) ||
						         (dwError == ERROR_WINHTTP_OPERATION_CANCELLED) || (dwError == ERROR_WINHTTP_LOGIN_FAILURE) ||
						         (dwError == ERROR_WINHTTP_INVALID_SERVER_RESPONSE) || (dwError == ERROR_WINHTTP_RESEND_REQUEST) ||
						         (dwError == ERROR_WINHTTP_SECURE_FAILURE) || (dwError == ERROR_WINHTTP_TIMEOUT))
						{
							callback(req, {{dwError, messageFromWininetCode(dwError)}});
							//throw io_exception(dwError,
							//                   0,
							//                   std::format("HttpSendRequest() Failed to {}; dwError={}",
							//                               argEndpoint,
							//                               messageFromWininetCode(dwError)));
						}
						else if (dwError == ERROR_WINHTTP_INVALID_URL)
						{
							callback(req, {{dwError, messageFromWininetCode(dwError)}});
							//throw(invalid_argument(std::format("HttpSendRequest() Failed Invalid URL {}; dwError={}",
							//                                   argEndpoint,
							//                                   messageFromWininetCode(dwError))));
						}
						else if (dwError != ERROR_FILE_NOT_FOUND)
						{
							nRetry = 0;
							//while (dwError == ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED)
							//{
							//	nError = WinHttpSetOption(
							//			hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);
							//
							//	dwError = ERROR_SUCCESS;
							//	nError  = WinHttpSendRequest(hRequest,
							//  !requestHeaders.empty() ? requestHeaders.c_str() : NULL,
							//  -1,
							//  argBody.has_value() ? (void*)argBody.value_or("").c_str() : NULL,
							//  DWORD(argBody.value_or("").length()),
							//  DWORD(argBody.value_or("").length()),
							//  NULL);
							//	dwError = GetLastError();
							//
							//	nRetry++;
							//
							//	if (nRetry > HTTPS_MAXRETRY) break;
							//}

							std::string rawResponse {};

							do
							{
								//memset(cBuf, '\0', sizeof(cBuf));
								hr = WinHttpReadData(hRequest, cBuf, MAXBUFSIZE - 1, &dwBytesRead)
											 ? S_OK
											 : HRESULT_FROM_WIN32(GetLastError());
#pragma warning(suppress : 6102)
								if (dwBytesRead)
								{
									cBuf[dwBytesRead] = '\0';
									rawResponse.append(cBuf, dwBytesRead);
								}
							} while (dwBytesRead > 0);

							// Log everything; DELETE usually does not return a body!
							hr = S_OK;
							resp.setContent(rawResponse);

							// Invoke the callback
							callback(req, resp);
						}
						else
						{
							callback(req, {{dwError, messageFromWininetCode(dwError)}});
							//hr = HRESULT_FROM_WIN32(dwError);
							//throw io_exception(dwError,
							//                   0,
							//                   std::format("HttpSendRequest() Failed to {}; dwError={}",
							//                               argEndpoint,
							//                               messageFromWininetCode(dwError)));
						}
					}
					else
					{
						callback(req, {{hr, std::format("HttpOpenRequest() failed; dwError:{}", hr)}});
						//throw io_exception(hr, 0, std::format("HttpOpenRequest() Failed; dwError={}", hr));
					}
				}
				else
				{
					callback(req, {{hr, std::format("WinHttpConnect() failed; dwError:{}", hr)}});
					//throw io_exception(hr, 0, std::format("InternetConnect() Failed; dwError={}", hr));
				}
			}
			else
			{
				callback(req, {{hr, std::format("WinHttpOpen() failed; dwError:{}", hr)}});
				//throw io_exception(hr, 0, std::format("InternetOpen() Failed; dwError={}", hr));
			}
		}

		/// @brief
		/// @param req
		/// @param callback
		void sendAsync(const RESTRequestType<>&                                                 req,
		               std::function<void(const RESTRequestType<>&, const RESTResponseType&)>&& callback)
		{
			throw std::exception(std::format("{} Not implemented.", __FUNCTION__).c_str());
		}
	};
} // namespace siddiqsoft


#endif // !RESTCLWINHTTP_HPP
