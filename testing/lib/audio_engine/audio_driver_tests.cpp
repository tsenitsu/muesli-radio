#include <gtest/gtest.h>

import std;
import audio_driver;

using namespace audio_engine;

const std::map<audio_driver::AudioDriver, std::string_view> audioDriverToString {
#ifdef _WIN32
        { audio_driver::AudioDriver::Wasapi,       "WASAPI"      },
        { audio_driver::AudioDriver::DirectSound,  "DirectSound" },
        { audio_driver::AudioDriver::WinMM,        "WinMM"       },
#endif
#ifdef __APPLE__
    { audio_driver::AudioDriver::CoreAudio,    "Core Audio"  },
#endif
#ifdef __linux__
        { audio_driver::AudioDriver::PulseAudio,   "PulseAudio"  },
        { audio_driver::AudioDriver::Alsa,         "ALSA"        },
        { audio_driver::AudioDriver::Jack,         "JACK"        },
#endif
        { audio_driver::AudioDriver::Null,         "Null"        }
    };

TEST(AudioDriverTests, toString) {
    for (auto const& [driver, driverString]: audioDriverToString) {
        EXPECT_EQ(audio_driver::toString(driver).value(), driverString);
    }

    EXPECT_EQ(audio_driver::toString(static_cast<audio_driver::AudioDriver>(55)), std::unexpected { "Audio driver unknown" });
}

TEST(AudioDriverTests, fromString) {
    for (auto driver: audio_driver::availableAudioDrivers) {
        if (auto found { audioDriverToString.find(driver) }; found != audioDriverToString.end()) {
            EXPECT_EQ(audio_driver::fromString(std::string { found->second }), driver);
        }
    }

    EXPECT_EQ(audio_driver::fromString("Core"), std::unexpected { "Audio backend unknown" });
}