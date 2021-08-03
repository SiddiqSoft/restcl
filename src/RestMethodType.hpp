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

	static void to_json(nlohmann::json& dest, const RESTMethodType& src)
	{
		switch (src)
		{
		case RESTMethodType::Get: dest = "GET"; break;
		case RESTMethodType::Patch: dest = "PATCH"; break;
		case RESTMethodType::Post: dest = "POST"; break;
		case RESTMethodType::Put: dest = "PUT"; break;
		case RESTMethodType::Delete: dest = "DELETE"; break;
		case RESTMethodType::Info: dest = "INFO"; break;
		default: dest = nullptr;
		}
	}

} // namespace siddiqsoft

#endif // !RESTMETHODTYPE_HPP
