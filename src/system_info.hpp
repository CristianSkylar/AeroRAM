#pragma once
#include "spec_types.hpp"
#include <atomic>
#include <functional>
namespace skylar {
SystemInfo readSystemInfo(const std::atomic_bool& cancelled,const std::function<void(const SystemInfo&)>& progress={});
}
