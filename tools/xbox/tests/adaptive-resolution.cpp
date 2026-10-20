// SPDX-FileCopyrightText: Copyright 2026 Jadyx Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cassert>
#include <cstdint>

#include "jadyx/game/adaptive_resolution.h"

int main() {
    using Jadyx::AdaptiveResolutionController;
    using Setup = Settings::ResolutionSetup;
    AdaptiveResolutionController policy{Setup::Res2X};

    // One slow window is insufficient; two consecutive windows lower one step.
    assert(!policy.Observe(18000, 1, Setup::Res1X));
    const auto reduced = policy.Observe(18000, 1, Setup::Res1X);
    assert(reduced && *reduced == Setup::Res3_4X);

    // Six fast windows are needed to restore one step.
    for (int i = 0; i < 5; ++i) assert(!policy.Observe(5000, 1, Setup::Res3_4X));
    const auto restored = policy.Observe(5000, 1, Setup::Res3_4X);
    assert(restored && *restored == Setup::Res1X);

    // The user's selected ceiling is respected.
    policy.SetCeiling(Setup::Res1X);
    for (int i = 0; i < 8; ++i) (void)policy.Observe(5000, 1, Setup::Res1X);
    assert(!policy.Observe(5000, 1, Setup::Res1X));

    // Empty windows reset the streak and never change settings.
    assert(!policy.Observe(100000, 0, Setup::Res1X));
    return 0;
}
