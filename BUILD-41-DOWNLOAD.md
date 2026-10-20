# Jadyx Xbox UWP — Build 41

The Build 41 Windows/UWP build completed successfully and the signed Xbox package was uploaded as a GitHub Actions artifact.

## Download

[Open the Build 41 workflow run](https://github.com/zimlegal6-byte/eden-xbox/actions/runs/39419033901), then scroll to **Artifacts** and download **`jadyx-xbox-uwp-41`**.

## What it contains

- The graphics menu and per-game internal-resolution work from its development branch.
- A conservative adaptive-resolution controller intended to reduce internal resolution when GPU load stays high, then restore it gradually when load improves.

## Important limitations

- The workflow build and package steps passed, but this build has **not** been verified on a physical Xbox Series S.
- FPS gains and game compatibility are not guaranteed.
- The automatic sibling-NSP update-import work is on a separate development branch and is **not included** in Build 41.
- GitHub Actions artifacts are temporary. This artifact is currently scheduled to expire on **November 3, 2026**. This page is a public download notice, not a permanent GitHub Release.

## Installation caution

Back up saves/settings before installing or changing versions. Do not uninstall an existing installation unless you have verified your save backup.