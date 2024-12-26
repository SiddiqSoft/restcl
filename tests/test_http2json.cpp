#include "gtest/gtest.h"
#include <iostream>
#include <barrier>
#include <version>

#include <string>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string_view>
#include <filesystem>

#include <iomanip>
#include <sstream>

#include <format>
#include "nlohmann/json.hpp"


#include "../include/private/http2json.hpp"

namespace siddiqsoft
{
    static std::string loadSampleFile(const std::string& fileName)
    {
        std::string samplesDirectoryPath {};


        if (auto env_samples_dir = std::getenv("SAMPLES_DIR"); env_samples_dir != nullptr) {
            std::clog << " -- Environment SAMPLES_DIR  : " << env_samples_dir << std::endl;
            samplesDirectoryPath = env_samples_dir;
        }
        else {
            std::clog << " -- Environment SAMPLES_DIR not found; cannot load " << fileName << std::endl;
        }

        if (std::filesystem::exists(samplesDirectoryPath)) {
            std::clog << " -- Using the samples directory at: " << samplesDirectoryPath << std::endl;
            std::clog << " -- Attempting to open the file   : " << std::format("{}/{}.http", samplesDirectoryPath, fileName)
                      << std::endl;

            try {
                std::stringstream testFile;
                std::ifstream     sampleInputFile {std::format("{}/{}.http", samplesDirectoryPath, fileName), std::ios::binary};

                if (sampleInputFile.is_open()) {
                    testFile << sampleInputFile.rdbuf();
                    sampleInputFile.close();
                }
                else {
                    throw std::runtime_error {std::format("Failed opening file: `{}`!", fileName)};
                }

                return testFile.str();
            }
            catch (std::exception& e) {
                std::cerr << "loadSampleFile exception: " << e.what() << std::endl;
                throw e;
            }
        }

        throw std::runtime_error {"Environment variable SAMPLES_DIR must point to directory for SIP samples!"};
    }

    // NOLINTNEXTLINE
    TEST(ImplementationChecks, Test_loadSampleFile)
    {
        auto contents = loadSampleFile("www-google-com");

        EXPECT_TRUE(contents.length() > 0);
    }

    // NOLINTNEXTLINE
    TEST(ImplementationChecks, Test_checkEnvironmentVars)
    {
        std::string samplesDirectoryPath {};
        auto        env_samples_dir = std::getenv("SAMPLES_DIR");

        EXPECT_TRUE(env_samples_dir != nullptr);

        if (env_samples_dir != nullptr) {
            samplesDirectoryPath = env_samples_dir;
        }

        EXPECT_TRUE(std::filesystem::exists(samplesDirectoryPath));
    }
} // namespace siddiqsoft