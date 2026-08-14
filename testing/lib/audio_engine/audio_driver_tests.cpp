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
        { audio_driver::AudioDriver::Jack,         "JACK"        },
        { audio_driver::AudioDriver::Alsa,         "ALSA"        },
#endif
     { audio_driver::AudioDriver::Null,         "Null (Silence)" }
    };

TEST(AudioDriverTests, toString) {
    for (auto const& [driver, driverString]: audioDriverToString) {
        EXPECT_EQ(audio_driver::toString(driver).value(), driverString);
    }

    EXPECT_EQ(audio_driver::toString(static_cast<audio_driver::AudioDriver>(55)), std::unexpected { "Audio driver unknown" });
}