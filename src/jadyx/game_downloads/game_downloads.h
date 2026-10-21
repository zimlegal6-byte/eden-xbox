// SPDX-FileCopyrightText: Copyright 2026 Jadyx Project
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Downloads a user-supplied, authorized homebrew NRO URL into the emulator's own LocalState\\games.
// This deliberately has no third-party game catalog, mirror scraper, login bypass, or source-specific
// resolver. The existing Web::Download implementation provides resumable .part files, cancellation,
// size caps, and optional checksum validation.

#include <cstdint>
#include <deque>
#include <filesystem>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <vector>

#include "jadyx/mods/work_queue.h"

namespace Jadyx::GameDownloads {
enum class Error : std::uint8_t { None, InvalidRequest, Space, Network, Checksum, TooLarge, Write, Cancelled, Conflict };
struct Request {
    std::string key;
    std::string title;
    std::string filename;  // Basename only; .nro.
    std::string url;       // Direct HTTPS URL from a source authorizing the download.
    std::string md5;       // Optional 32-character MD5 supplied by the source.
    std::uint64_t size{};  // Expected size, zero when unknown.
};
struct Status {
    std::string key, title;
    bool running{};
    std::uint64_t done{}, total{}, speed{};
    Error error{Error::None};
};
struct Finished { Request request; std::filesystem::path file; Error error{Error::None}; };

/// Threaded downloader writing only to the supplied games folder.
/// Pass ApplicationData.Current.LocalFolder/games from the Jadyx process.
class Manager {
public:
    explicit Manager(std::filesystem::path games_folder);
    ~Manager();
    bool Enqueue(Request request, std::string& error);
    void CancelAll();
    [[nodiscard]] std::vector<Status> GetStatus() const;
    [[nodiscard]] bool Active() const;
    /// Consumes one completion and removes its status entry, allowing a later retry.\n    std::optional<Finished> TakeFinished();
private:
    void Run(Request request, std::stop_token stop);
    std::filesystem::path games_folder;
    mutable std::mutex mutex;
    std::vector<Status> status;
    std::deque<Finished> finished;
    std::stop_source cancel;
    Mods::WorkQueue queue;
};
} // namespace Jadyx::GameDownloads
