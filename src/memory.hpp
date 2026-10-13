#pragma once
#include "core.hpp"
#include <windows.h>
#include <string>
namespace skylar {
MemorySample readMemory();
double appMemoryMB();
struct CleanOptions { bool cache = true, apps = true; };
struct CleanResult {
    MemorySample before{}, after{};
    int requested = 0, succeeded = 0;
    std::wstring details;
};
CleanResult cleanMemory(CleanOptions options);
}
