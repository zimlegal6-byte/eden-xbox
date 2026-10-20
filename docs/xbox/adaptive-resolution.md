# Adaptive resolution on Xbox

## Scope

The in-game Display page can enable **Resolução adaptativa** per title. When enabled, the
frontend samples the existing D3D12 GPU-time counters and guest-frame queue count every five
seconds. A small deterministic controller changes the existing Eden internal-resolution setting
on the GPU thread. It does not call an external AI service and adds no work to the draw hot path.

## Policy

- Reduce one supported resolution step after two consecutive windows averaging at least 17 ms
  of GPU timestamp time per queued guest frame.
- Restore one step only after six consecutive windows at or below 8.5 ms per guest frame.
- The ceiling is the scale selected by the player at the start of the session or most recently
  chosen manually in the menu. The controller never upscales beyond that ceiling.
- Sampling pauses while the in-game menu is open, so overlay work is not attributed to the game.
- A title with no guest frames in a window cannot trigger a scale change.
- The setting is persisted per title. The controller's transient scale is not written over the
  player's chosen baseline; the next launch starts from that baseline and adapts again.

## Safety and limitations

This is a conservative adaptive heuristic, not a trained model. It changes only internal resolution,
which the existing renderer already supports. It does not promise higher FPS: reducing resolution
helps only when GPU work is the bottleneck, and GPU timestamps can be unavailable or imperfect.
CPU/JIT stalls, shader compilation, memory exhaustion and renderer incompatibilities need separate
diagnostics. All changes are reversible by disabling the option or selecting a manual scale.

## Validation gates

1. GitHub Actions must compile and package the UWP target.
2. Run the standalone policy regression test.
3. On a Series S, compare the same gameplay route with the option off/on; capture GPU time per
   guest frame, FPS/frame pacing, chosen scale, memory headroom and graphics errors.
4. Do not call the feature validated until a physical-console run confirms the display changes
   and no visual or stability regressions.
