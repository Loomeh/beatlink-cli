#pragma once

#include <cstdint>
#include <sys/types.h>
#include <string>
#include "writemem.hpp"

class Memory {
public:
    // Applies all memory patches to the target process
    void ApplyPatches(WriteMemory& writer, pid_t pid, uint64_t baseAddress, std::string newAuthCode);
};
