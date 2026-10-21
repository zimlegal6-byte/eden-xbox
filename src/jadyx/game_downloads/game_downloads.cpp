// SPDX-FileCopyrightText: Copyright 2026 Jadyx Project
// SPDX-License-Identifier: GPL-3.0-or-later
#include "jadyx/game_downloads/game_downloads.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <thread>
#include "common/logging.h"
#include "jadyx/mods/mod_web.h"

namespace Jadyx::GameDownloads {
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;
constexpr std::uint64_t SpaceMargin = 256ull << 20;
constexpr std::uint64_t MaxGameFile = 32ull << 30;

namespace {
bool SafeFilename(const std::string& name) {
    if (name.empty() || name.size() > 180 || name == "." || name == "..") return false;
    if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos ||
        name.find(':') != std::string::npos || name.find("..") != std::string::npos) return false;
    const auto dot = name.rfind('.');
    if (dot == std::string::npos) return false;
    std::string ext = name.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".nsp" || ext == ".xci" || ext == ".nro";
}
bool SafeRequest(const Request& r) {
    if (r.key.empty() || r.key.size() > 128 || r.title.empty() || !SafeFilename(r.filename)) return false;
    if (r.url.rfind("https://", 0) != 0 || r.url.size() > 4096) return false;
    if (!r.md5.empty() && (r.md5.size() != 32 ||
        !std::all_of(r.md5.begin(), r.md5.end(), [](unsigned char c) { return std::isxdigit(c); }))) return false;
    return r.size <= MaxGameFile;
}
bool Wait(int seconds, std::stop_token stop) {
    const auto until = Clock::now() + std::chrono::seconds{seconds};
    while (Clock::now() < until) {
        if (stop.stop_requested()) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }
    return !stop.stop_requested();
}
Error Convert(Mods::Web::DownloadError e) {
    switch (e) {
    case Mods::Web::DownloadError::None: return Error::None;
    case Mods::Web::DownloadError::Network: return Error::Network;
    case Mods::Web::DownloadError::Checksum: return Error::Checksum;
    case Mods::Web::DownloadError::TooLarge: return Error::TooLarge;
    case Mods::Web::DownloadError::Write: return Error::Write;
    case Mods::Web::DownloadError::Cancelled: return Error::Cancelled;
    }
    return Error::Network;
}
} // namespace

Manager::Manager(fs::path folder) : games_folder{std::move(folder)} {}
Manager::~Manager() { CancelAll(); }

bool Manager::Enqueue(Request request, std::string& error) {
    if (!SafeRequest(request)) {
        error = "Use a direct HTTPS URL and a safe .nsp, .xci or .nro filename.";
        return false;
    }
    {
        const std::scoped_lock lock{mutex};
        if (std::any_of(status.begin(), status.end(), [&](const Status& s) { return s.key == request.key; })) {
            error = "This download is already queued or running.";
            return false;
        }
        status.push_back({request.key, request.title, false, 0, request.size, 0, Error::None});
    }
    queue.Post([this, request = std::move(request)](std::stop_token stop) { Run(request, stop); });
    return true;
}
void Manager::CancelAll() {
    queue.Clear();
    const std::scoped_lock lock{mutex};
    cancel.request_stop();
    cancel = std::stop_source{};
    std::erase_if(status, [](const Status& s) { return !s.running; });
}
std::vector<Status> Manager::GetStatus() const { const std::scoped_lock lock{mutex}; return status; }
bool Manager::Active() const { const std::scoped_lock lock{mutex}; return !status.empty(); }
std::optional<Finished> Manager::TakeFinished() {
    const std::scoped_lock lock{mutex};
    if (finished.empty()) return std::nullopt;
    auto item = std::move(finished.front()); finished.pop_front(); return item;
}

void Manager::Run(Request request, std::stop_token stop) {
    std::stop_source local;
    std::stop_token cancelled;
    {
        const std::scoped_lock lock{mutex};
        cancelled = cancel.get_token();
        for (auto& s : status) if (s.key == request.key) s.running = true;
    }
    const std::stop_callback on_stop{stop, [&] { local.request_stop(); }};
    const std::stop_callback on_cancel{cancelled, [&] { local.request_stop(); }};
    std::error_code ec;
    fs::create_directories(games_folder, ec);
    const auto target = games_folder / request.filename;
    auto part = target; part += ".part";
    Error error = Error::None;
    const auto space = fs::space(games_folder, ec);
    if (ec || space.available < SpaceMargin) error = Error::Space;
    else if (fs::exists(target, ec)) error = Error::Conflict; // Never overwrite an existing game.
    else {
        const auto existing = fs::exists(part, ec) ? fs::file_size(part, ec) : 0;
        const auto limit = std::min(MaxGameFile, space.available - SpaceMargin + existing);
        auto sample_time = Clock::now();
        std::uint64_t sample_bytes = existing;
        double smoothed_speed = 0;
        const auto progress = [&](std::uint64_t done, std::uint64_t total) {
            const auto now = Clock::now();
            const double seconds = std::chrono::duration<double>(now - sample_time).count();
            if (seconds >= 0.5) {
                const double current = static_cast<double>(done - std::min(done, sample_bytes)) / seconds;
                smoothed_speed = smoothed_speed == 0 ? current : smoothed_speed * 0.7 + current * 0.3;
                sample_time = now; sample_bytes = done;
            }
            const std::scoped_lock lock{mutex};
            for (auto& s : status) if (s.key == request.key) {
                s.done = done; s.total = total; s.speed = static_cast<std::uint64_t>(smoothed_speed);
            }
        };
        std::string detail;
        Mods::Web::DownloadError result = Mods::Web::DownloadError::None;
        for (int attempt = 0; attempt <= 3; ++attempt) {
            if (attempt > 0 && !Wait(1 << attempt, local.get_token())) {
                result = Mods::Web::DownloadError::Cancelled; break;
            }
            result = Mods::Web::Download(request.url, part, request.md5, limit, local.get_token(), progress, detail);
            if (result != Mods::Web::DownloadError::Network || local.stop_requested()) break;
        }
        if (result == Mods::Web::DownloadError::Network && local.stop_requested()) result = Mods::Web::DownloadError::Cancelled;
        error = Convert(result);
        if (error == Error::None) {
            fs::rename(part, target, ec);
            if (ec) error = Error::Write;
        }
        if (error != Error::None && error != Error::Cancelled)
            LOG_WARNING(Frontend, "Game download failed: {} ({})", request.key, detail);
        else if (error == Error::None)
            LOG_INFO(Frontend, "Game download completed: {} bytes={}", request.filename, fs::file_size(target, ec));
    }
    const std::scoped_lock lock{mutex};
    for (auto& s : status) if (s.key == request.key) { s.running = false; s.error = error; }
    finished.push_back({std::move(request), error == Error::None ? target : fs::path{}, error});
}
} // namespace Jadyx::GameDownloads
