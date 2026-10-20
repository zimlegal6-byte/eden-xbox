// SPDX-FileCopyrightText: Copyright 2026 Jadyx Project
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <optional>

#include "common/settings.h"

namespace Jadyx {

/// Conservative, renderer-backed dynamic resolution policy. It reacts only to GPU time per
/// guest frame; it never changes guest speed, clocks, shaders, or save data. Call at a slow
/// cadence (the frontend uses five-second windows), not on the render hot path.
class AdaptiveResolutionController {
public:
    using Setup = Settings::ResolutionSetup;

    explicit AdaptiveResolutionController(Setup ceiling = Setup::Res1X) : ceiling{ceiling} {}

    void SetCeiling(Setup setup) {
        const int value = Clamp(Index(setup));
        ceiling = static_cast<Setup>(value);
        if (Index(current_hint) > value) current_hint = ceiling;
        slow_windows = 0;
        fast_windows = 0;
    }

    /// Returns a new scale only after sustained evidence. gpu_busy_delta_us is the sum of
    /// GPU timestamp durations in the window and guest_frames_delta is the number of guest
    /// frames queued in that same window. Empty windows are ignored.
    std::optional<Setup> Observe(std::uint64_t gpu_busy_delta_us, std::uint64_t guest_frames_delta,
                                 Setup current) {
        const int current_index = Clamp(Index(current));
        const int ceiling_index = Clamp(Index(ceiling));
        if (guest_frames_delta == 0) {
            slow_windows = fast_windows = 0;
            return std::nullopt;
        }

        const std::uint64_t gpu_us_per_frame = gpu_busy_delta_us / guest_frames_delta;
        if (gpu_us_per_frame >= ReduceThresholdUs) {
            ++slow_windows;
            fast_windows = 0;
        } else if (gpu_us_per_frame <= RestoreThresholdUs) {
            ++fast_windows;
            slow_windows = 0;
        } else {
            slow_windows = fast_windows = 0;
        }

        // Two consecutive slow windows before reducing detail; six fast windows before
        // restoring it. Hysteresis prevents the scale from oscillating around one threshold.
        if (slow_windows >= 2 && current_index > MinIndex) {
            slow_windows = fast_windows = 0;
            current_hint = static_cast<Setup>(current_index - 1);
            return current_hint;
        }
        if (fast_windows >= 6 && current_index < ceiling_index) {
            slow_windows = fast_windows = 0;
            current_hint = static_cast<Setup>(current_index + 1);
            return current_hint;
        }
        return std::nullopt;
    }

private:
    static constexpr int MinIndex = 0;  // 0.25x
    static constexpr int MaxIndex = 12; // 8x
    static constexpr std::uint64_t ReduceThresholdUs = 17000;
    static constexpr std::uint64_t RestoreThresholdUs = 8500;

    static int Index(Setup setup) { return static_cast<int>(setup); }
    static int Clamp(int value) { return value < MinIndex ? MinIndex : value > MaxIndex ? MaxIndex : value; }

    Setup ceiling;
    Setup current_hint{Setup::Res1X};
    unsigned slow_windows{};
    unsigned fast_windows{};
};

} // namespace Jadyx
