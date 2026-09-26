#pragma once
#include "audio_pcm.h"
namespace ssc_audio {
struct OggInfo {unsigned rate=0,channels=0;double seconds=0;};
OggInfo ogg_info(const std::filesystem::path& path);
Wave read_ogg(const std::filesystem::path& path);
void write_ogg(const std::filesystem::path& path,const Wave& wave);
}
