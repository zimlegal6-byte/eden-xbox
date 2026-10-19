// SPDX-FileCopyrightText: Copyright 2026 Jadyx Project
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace Jadyx::Recovery {

// Deliberately separates guest progress from frames presented to the display.
// A nonzero guest frame count alone is NOT proof that the screen is rendering.
enum class Result {
    Observing,
    PassGraphics,
    PassInteractive,
    FailAudioOnly,
    FailNoPresent,
    FailFrozen,
    FailCrash,
    Inconclusive,
};

// Numeric IDs are persisted per Switch Program ID; never reorder or reuse these values.
enum class Profile : std::uint32_t {
    Baseline = 0,
    LongerFirstFrameWait = 1,
    SynchronousPresent = 2,
    ForceSwapIntervalOne = 3,
};

constexpr std::uint32_t Id(Profile profile) noexcept {
    return static_cast<std::uint32_t>(profile);
}

constexpr Profile ProfileFromId(std::uint32_t id) noexcept {
    switch (id) {
    case 1: return Profile::LongerFirstFrameWait;
    case 2: return Profile::SynchronousPresent;
    case 3: return Profile::ForceSwapIntervalOne;
    default: return Profile::Baseline;
    }
}

struct Sample {
    std::chrono::steady_clock::time_point time{};
    std::uint64_t presented_frames{};
    std::uint64_t guest_frames{};
    bool audio_activity_known{};
    bool audio_active{};
    bool crashed{};
    bool gpu_error{};
    bool input_progress_confirmed{};
};

struct Summary {
    Result result{Result::Observing};
    std::optional<double> presented_fps;
    std::optional<double> guest_fps;
    double observation_seconds{};
    std::uint64_t presented_delta{};
    std::uint64_t guest_delta{};
};

constexpr std::size_t MaxAttempts = 4;
constexpr std::chrono::seconds ObservationWindow{12};
constexpr std::chrono::seconds LongerFirstFrameWindow{30};

// Bounded, deterministic plan. This class only chooses a candidate profile;
// the caller must apply it using supported renderer APIs, restart the game if
// required, record the attempt, and restore saved settings on failure.
class Controller {
public:
    constexpr std::size_t AttemptCount() const noexcept { return MaxAttempts; }

    constexpr Profile ProfileFor(std::size_t attempt) const noexcept {
        switch (attempt) {
        case 0: return Profile::Baseline;
        case 1: return Profile::LongerFirstFrameWait;
        case 2: return Profile::SynchronousPresent;
        default: return Profile::ForceSwapIntervalOne;
        }
    }

    constexpr bool ShouldStop(Result result) const noexcept {
        return result == Result::PassGraphics || result == Result::PassInteractive ||
               result == Result::FailCrash;
    }

    static Summary Evaluate(const Sample& before, const Sample& after,
                            std::chrono::seconds minimum_window = ObservationWindow) {
        Summary out{};
        if (after.time <= before.time) {
            out.result = Result::Inconclusive;
            return out;
        }

        const auto elapsed =
            std::chrono::duration<double>(after.time - before.time).count();
        out.observation_seconds = elapsed;
        out.presented_delta = after.presented_frames >= before.presented_frames
                                  ? after.presented_frames - before.presented_frames : 0;
        out.guest_delta = after.guest_frames >= before.guest_frames
                              ? after.guest_frames - before.guest_frames : 0;

        if (elapsed > 0.0) {
            out.presented_fps = static_cast<double>(out.presented_delta) / elapsed;
            out.guest_fps = static_cast<double>(out.guest_delta) / elapsed;
        }
        if (after.crashed) {
            out.result = Result::FailCrash;
        } else if (after.gpu_error) {
            out.result = Result::FailFrozen;
        } else if (elapsed < static_cast<double>(minimum_window.count())) {
            out.result = Result::Observing;
        } else if (out.presented_delta > 0) {
            out.result = after.input_progress_confirmed ? Result::PassInteractive
                                                        : Result::PassGraphics;
        } else if (after.audio_activity_known && after.audio_active) {
            out.result = Result::FailAudioOnly;
        } else if (out.guest_delta > 0) {
            // Guest is advancing but there is no reliable evidence of display output.
            out.result = Result::FailNoPresent;
        } else {
            out.result = Result::Inconclusive;
        }
        return out;
    }

    static constexpr std::string_view Name(Result result) noexcept {
        switch (result) {
        case Result::Observing: return "OBSERVING";
        case Result::PassGraphics: return "PASS_GRAPHICS";
        case Result::PassInteractive: return "PASS_INTERACTIVE";
        case Result::FailAudioOnly: return "FAIL_AUDIO_ONLY";
        case Result::FailNoPresent: return "FAIL_NO_PRESENT";
        case Result::FailFrozen: return "FAIL_FROZEN";
        case Result::FailCrash: return "FAIL_CRASH";
        default: return "INCONCLUSIVE";
        }
    }
};

} // namespace Jadyx::Recovery
