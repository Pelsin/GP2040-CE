#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include "config.pb.h"
#include <stdint.h>
#include <string>

struct FlashStats
{
	uint64_t setHasFlagsUs;
	uint64_t encodeUs;
	uint64_t crcUs;
	uint64_t commitUs;
	uint32_t writeCount;
};

extern FlashStats flashStats;

namespace ConfigUtils {
    void load(Config& config);
    bool save(Config& config);

    void initUnsetPropertiesWithDefaults(Config& config);

    std::string toJSON(const Config& config);
    bool fromJSON(Config& config, const char* data, size_t dataLen);
    bool fromLegacyStorage(Config& config);
}

#endif
