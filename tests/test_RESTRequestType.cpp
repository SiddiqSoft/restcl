#include "gtest/gtest.h"
#include <iostream>

#include "nlohmann/json.hpp"
#include "../src/restcl.hpp"
#include "../src/RESTRequestType.hpp"

namespace siddiqsoft
{
	TEST(TRestRequest, test1a)
	{
		RESTRequestType srt {RESTMethodType::Get, "https://www.siddiqsoft.com/"};

		nlohmann::json doc(srt);
		
		// Checks the implementation of the json implementation
		std::cerr << "Serialized json: " << doc.dump(3) << std::endl;
	}


	TEST(TRestRequest, test1b)
	{
		RESTRequestType srt {RESTMethodType::Get, "https://www.siddiqsoft.com/"};

		// Checks the implementation of the encode() implementation
		std::cerr << "Wire serialize              : " << srt.encode() << std::endl;
	}


	TEST(TRestRequest, test1c)
	{
		RESTRequestType srt {RESTMethodType::Get, "https://www.siddiqsoft.com/"};

		// Checks the implementation of the std::format implementation
		std::cerr << std::format("Wire serialize              : {}\n", srt);
	}

} // namespace siddiqsoft