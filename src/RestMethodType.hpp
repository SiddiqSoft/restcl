#pragma once
#ifndef RESTMETHODTYPE_HPP
#define RESTMETHODTYPE_HPP

#include "nlohmann/json.hpp"


namespace siddiqsoft
{
	enum class RESTMethodType
	{
		Get,
		Patch,
		Post,
		Put,
		Delete,
		Info,
		Unknown
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(RESTMethodType,
	                             {{RESTMethodType::Get, "GET"},
	                              {RESTMethodType::Patch, "PATCH"},
	                              {RESTMethodType::Post, "POST"},
	                              {RESTMethodType::Put, "PUT"},
	                              {RESTMethodType::Delete, "DELETE"},
	                              {RESTMethodType::Info, "INFO"},
	                              {RESTMethodType::Unknown, nullptr}});


} // namespace siddiqsoft

#endif // !RESTMETHODTYPE_HPP
