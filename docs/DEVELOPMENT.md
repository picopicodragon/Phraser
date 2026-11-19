# PC1 v0.5 acceptance tests

The `feature/v05-waveform-16slots-arrangement` branch is experimental until Windows MSVC and Ableton Live 12 validate it.

## Build

```powershell
cd C:\Users\picopicoDragon\Documents\Phraser
git fetch origin
git switch -c feature/v05-waveform-16slots-arrangement --track origin/feature/v05-waveform-16slots-arrangement
cmake -S . -B build-v05 -G "Visual Studio 18 2026" -A x64
cmake --build build-v05 --config Release --target Phraser_VST3
```

If the local branch already exists, use `git switch feature/v05-waveform-16slots-arrangement` then `git pull`. Preserve the installed v0.4 VST3 before copying the new bundle.

## Acceptance checklist

- [ ] Windows MSVC Release builds VST3 without errors/C4819.
- [ ] Ableton Live 12 loads plugin and displays all 16 WAV lanes in the 1460×950 GUI.
- [ ] Independently load 16 short WAVs, different sounds and peak shapes.
- [ ] A sample's playback marker moves and disappears after its voice ends.
- [ ] Different waveforms visibly differ; missing slots show a flat line.
- [ ] Select a different sample, change START FROM/TO and LENGTH MIN/MAX; reopen selection and confirm ranges remain.
- [ ] Start range 0.0–0.1 produces beginnings; range 0.8–1.0 produces late slices.
- [ ] Length range 0.01–0.02 produces shorter slices than 0.5–0.8 (before SIZE scaling).
- [ ] 1 bar × 8 = 8 active steps; 4 bars × 8 = 32; 4 bars × 16 = 64.
- [ ] Host BPM change keeps intended 8/16 step subdivisions in 4/4.
- [ ] 64-step grid cells mute, lock and shift-route to selected sample.
- [ ] Keep A/B restores full 64-step snapshot.
- [ ] Live session save/reopen restores all 16 WAV paths, per-slot range values, patterns and parameters.
- [ ] Reopen a project from v0.4 with four WAVs and a 16-step phrase.
- [ ] Missing/moved WAV path produces silence rather than crash.
- [ ] Transport stop/seek, loop and 44.1/48k playback do not crash.
- [ ] Test high-DPI monitor for clipping; CPU and memory with 16 WAVs.

## Known technical debt

Synchronous decoding; immutable shared_ptr ownership in audio callback; approximate onset detector; no independent time-stretch; sample peak visualizer only; fixed 4/4; no clip export; no WAV memory budget across slots; no automated audio tests.
