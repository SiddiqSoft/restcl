#include "gtest/gtest.h"
#include <iostream>

#include "nlohmann/json.hpp"
#include "../src/RESTClient.hpp"

namespace siddiqsoft
{
	TEST(TRestRequest, test1a)
	{
		auto srt = "https://www.siddiqsoft.com/"_GET;

		nlohmann::json doc(srt);

		// Checks the implementation of the json implementation
		std::cerr << "Serialized json: " << doc.dump(3) << std::endl;
	}


	TEST(TRestRequest, test1b)
	{
		auto srt = "https://www.siddiqsoft.com/"_GET;

		// Checks the implementation of the encode() implementation
		std::cerr << "Wire serialize              : " << srt.encode() << std::endl;
	}


	TEST(TRestRequest, test1c)
	{
		auto srt = "https://www.siddiqsoft.com/"_GET;

		// Checks the implementation of the std::format implementation
		std::cerr << std::format("Wire serialize              : {}\n", srt);
	}

} // namespace siddiqsoft