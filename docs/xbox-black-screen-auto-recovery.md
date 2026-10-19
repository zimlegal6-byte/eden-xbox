# Xbox black-screen diagnosis and automatic recovery

Status: implementation plan/scaffold. This document is not proof that automatic recovery is already implemented or tested.

## Goal

On Xbox Series S/UWP, run bounded, reversible diagnostic attempts for a game that starts with a black screen or poor performance. Prefer a confirmed working result, not blindly changing settings until something appears to work.

## Test result states

- `PASS_GRAPHICS`: new frames are presented over a stable observation window and frame timing is measurable.
- `PASS_INTERACTIVE`: graphics pass plus a game-specific input/progress check. This must not be inferred from FPS alone.
- `FAIL_AUDIO_ONLY`: audio activity is positively observable, but no new frame is presented before timeout.
- `FAIL_NO_PRESENT`: no frame presented; audio state may be unknown.
- `FAIL_FROZEN`: frames stop advancing or the process stops responding.
- `FAIL_CRASH`: fatal error, exception, or process exit.
- `INCONCLUSIVE`: telemetry cannot distinguish loading, menu, video, and failure reliably.

## Telemetry required

Collect title/program ID, app commit/build, start time, time to first present, presented-frame counter, frame-time/FPS samples, GPU/renderer and pipeline/shader errors, swapchain/present failures, memory-pressure events, emulator exceptions, and audio activity only where a trustworthy signal exists. Persist a bounded report and log in app LocalState. Never claim audio is active if the current audio backend exposes no reliable activity signal.

## Bounded recovery ladder

Run one change per attempt, record the before/after result, and restore the baseline if the attempt fails. Stop on success, crash, repeated identical failure, or attempt limit.

1. Baseline: launch with current per-title and global settings; capture telemetry.
2. Wait/retry: allow a longer first-frame timeout once to avoid mistaking shader compilation/loading for a black-screen fault.
3. Clear only regenerable shader/pipeline caches using existing supported cache APIs; never delete saves or user content.
4. Try known renderer-compatible, reversible per-title graphics profiles only when those settings already exist and can be applied safely. Keep a backup and log every change.
5. Try conservative performance presets only when the app has explicit supported controls for them; do not change unsupported settings or assume a higher FPS limit fixes a missing render path.
6. Re-launch and re-run the same observation window. If no profile succeeds, restore baseline and export diagnostics for code-level investigation.

No random toggling, unbounded loops, firmware/keys modification, downloading unknown patches, or changing game files. Automatic recovery must be opt-in initially and provide a one-button restore/reset.

## FPS and performance rules

Measure presented frames over a monotonic time interval; report average FPS and frame-time percentiles only when enough samples exist. Separate emulation speed, guest frame rate, and host-present rate if each is measurable. Increasing an FPS cap does not create performance. A higher-resolution/upscaling option may change GPU load but can also worsen compatibility; only try documented settings and verify the result.

## UI/report

Show attempt number, profile used, result classification, first-frame time, FPS/frame-time, and a short probable-cause explanation with confidence (low/medium/high). Offer `Retry`, `Stop`, `Restore original settings`, and `Export report`.

## Implementation sequence

1. Instrument the existing UWP render/present path and connect it to a reliable presented-frame counter.
2. Add bounded watchdog/classification and diagnostic report, without changing settings automatically.
3. Add a small recovery controller using existing supported settings and cache APIs.
4. Add FPS/performance profiles only after verifying which options actually affect this UWP build.
5. Build and test on Xbox Series S. A GitHub Actions compile cannot validate rendering or gameplay.
6. Validate with Luigi's Mansion 3 first and record results. Do not mark a title fixed until tested on hardware.

## Acceptance criteria

- Every attempt is bounded and logged.
- A failed attempt restores original per-title settings.
- No save data or game content is deleted.
- The report distinguishes measured facts from suspected causes.
- A graphics pass is not mislabeled as full gameplay compatibility.
- No automatic recovery claims success without observing new presented frames; interactive pass requires a separate progress/input check.
