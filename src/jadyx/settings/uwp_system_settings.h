// SPDX-FileCopyrightText: Copyright 2026 Jadyx Project
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include "jadyx/settings/system_settings.h"

namespace Service::Account {
class ProfileManager;
}

namespace Jadyx {
/// Saved in the app's LocalSettings, like the controller options.
SystemSettings LoadSystemSettings();
bool SaveSystemSettings(const SystemSettings& settings);

/// Windows' preferred languages and home region (the console's, on Xbox). Never throws.
SystemLocale DetectSystemLocale();

/// Resolves the saved settings against the system and makes the result current: the interface
/// language for the frontend, and Eden's system language and region for the next game.
ResolvedSystem ApplySystemSettings();

/// Makes upscaling Eden's scaling settings, which the D3D12 present reads every frame. On the GPU
/// thread while a game runs (GPU::RunOnGpuThread), the thread that reads them.
void ApplyUpscaling(Upscaling upscaling);

// Per-title upscaling overrides, keyed by the Switch program ID.
// Missing or invalid entries follow the global preference.
Upscaling LoadGameUpscaling(std::uint64_t program_id, Upscaling fallback);
bool SaveGameUpscaling(std::uint64_t program_id, Upscaling upscaling);

// Per-title internal resolution scale (the real renderer ResolutionSetup enum).
Settings::ResolutionSetup LoadGameResolutionSetup(std::uint64_t program_id, Settings::ResolutionSetup fallback);
bool SaveGameResolutionSetup(std::uint64_t program_id, Settings::ResolutionSetup setup);

// Per-title adaptive resolution preference; stored separately from game saves.
bool LoadGameAdaptiveResolution(std::uint64_t program_id, bool fallback = false);
bool SaveGameAdaptiveResolution(std::uint64_t program_id, bool enabled);

// Per-title black-screen recovery profile, stored independently from upscaling and game saves.
// Values are the stable numeric Profile IDs defined by the recovery controller (0..3).
// Invalid/missing values return fallback; no game content or save data is touched.
std::uint32_t LoadGameRecoveryProfile(std::uint64_t program_id, std::uint32_t fallback = 0);
bool SaveGameRecoveryProfile(std::uint64_t program_id, std::uint32_t profile);

/// Renames the active profile to the saved user name (the default one when none is saved), before
/// a game starts. Returns false when the profile could not be written.
bool ApplyUserName(Service::Account::ProfileManager& profiles, std::wstring_view name);
} // namespace Jadyx
