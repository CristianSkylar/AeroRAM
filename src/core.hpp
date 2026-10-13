#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace skylar {
constexpr double GiB = 1073741824.0;
constexpr double MiB = 1048576.0;
struct MemorySample {
    bool valid = false;
    uint64_t total = 0, available = 0, commitTotal = 0, commitAvailable = 0, cache = 0;
    double usedPercent() const {
        if (!valid || !total) return 0;
        return 100.0 * (1.0 - double(std::min(available, total)) / double(total));
    }
    uint64_t used() const { return total - std::min(total, available); }
};
inline int64_t availableChange(const MemorySample& before, const MemorySample& after) {
    if (!before.valid || !after.valid) return 0;
    const auto cap = uint64_t(std::numeric_limits<int64_t>::max());
    if (after.available >= before.available)
        return int64_t(std::min(after.available - before.available, cap));
    return -int64_t(std::min(before.available - after.available, cap));
}
enum class Outcome { Failed, Partial, Complete };
inline Outcome outcome(int requested, int succeeded) {
    if (requested <= 0 || succeeded <= 0) return Outcome::Failed;
    return succeeded >= requested ? Outcome::Complete : Outcome::Partial;
}
inline double smoothToward(double current, double target, double dt, double speed = 9.0) {
    if (!std::isfinite(target)) return current;
    dt = std::clamp(dt, 0.0, 0.1);
    const double result = current + (target-current) * (1.0 - std::exp(-speed*dt));
    return std::abs(result-target) < .015 ? target : result;
}
struct History {
    std::array<double, 60> values{};
    size_t count = 0;
    void push(double p) {
        p = std::clamp(p, 0.0, 100.0);
        if (count < values.size()) values[count++] = p;
        else { std::move(values.begin()+1, values.end(), values.begin()); values.back() = p; }
    }
};
}
