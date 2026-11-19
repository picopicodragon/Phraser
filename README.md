# Phraser

Phraser is an independent Windows x64 VST3 sample-phrase generator for Ableton Live 12, built with C++17 and JUCE 8.0.8.

## Version status

- `main` contains the earlier tested v0.2 prototype.
- `feature/phrase-engine-v03` contains v0.3.
- `feature/v04-orbit-ui-pitch` contains v0.4.
- `feature/v05-waveform-16slots-arrangement` contains **experimental v0.5 source**. It has **not been compiled or tested on Windows** in this change.

## v0.5 features

- **16 WAV slots**. Each sample is mono/stereo and limited to 4,000,000 frames (roughly 91 sec at 44.1 kHz). Load independent audio into each slot.
- **16 waveform thumbnails** with lightweight offline-computed peak envelopes and approximate real-time playback position indicators.
- **Independent random windows per sample**: START FROM, START TO, LENGTH MIN and LENGTH MAX. Select a waveform lane, change its ranges, then press GENERATE PHRASE. Each new event draws a random start and duration using the settings of its assigned sample.
- **1–4 bars** and **8 or 16 steps per bar**. This yields 8–64 total steps, including **4 bars × 8 = 32 steps** or **4 bars × 16 = 64 steps**.
- A crisp, square-edged, dark graphite production interface with compact waveform rows, vivid lime playheads and a dense 4×16 sequence grid. No website runtime and no copied original phrz assets.
- Click a sequence cell to toggle it, Shift+click assigns the currently selected WAV to it, right-click locks it during regeneration.
- Existing Density, Size, Breaks, Swing, Output, Fade, Snap, Variation, Pitch, Pitch Spread, Keep A/B and 8-voice short-release overlap.
- Project state stores 16 WAV paths, the 64-step pattern, A/B memories and each sample's random windows. Older 16-step sessions are loaded with the first 16 steps preserved.

## Build

Requires x64 Windows, Visual Studio 2026 with C++ desktop workload, CMake 3.22+, Git.

```powershell
cd C:\Users\picopicoDragon\Documents\Phraser
git fetch origin
git switch -c feature/v05-waveform-16slots-arrangement --track origin/feature/v05-waveform-16slots-arrangement
cmake -S . -B build-v05 -G "Visual Studio 18 2026" -A x64
cmake --build build-v05 --config Release --target Phraser_VST3
```

Output is the directory `build-v05/Phraser_artefacts/Release/VST3/Phraser.vst3`. Keep a backup of the working prior VST3, quit Live and copy the entire new bundle to `C:\Program Files\Common Files\VST3\Phraser.vst3` using administrator rights. Rescan VST3 in Live. Don't overwrite the working copy until build success and initial checks.

## How to use

1. Load up to 16 WAVs from the 16 source buttons, each with its waveform lane.
2. Click a waveform to select its lane. Set its START FROM/TO and LENGTH MIN/MAX using the inspector on the right. Values are normalized fractions of each WAV's duration.
3. Set BARS to 1–4 and STEPS/BAR to 8 or 16. Four bars and eight steps/bar gives a 32-step sequence.
4. Adjust Density / Size / Breaks / Snap / Variation and press GENERATE PHRASE. A fresh pattern is built for the current sequence length. The overall SIZE knob scales each independently chosen sample length; 0.25 is neutral.
5. Start Live transport. A moving marker over the WAV lane indicates its approximate playback position. The sequence grid highlights the current step.
6. Use the bottom grid to mute, route or lock events, and Keep A/B to store phrase alternatives.

## Limitations

- This is an experimental, **unverified** v0.5 branch, not a release.
- The 16 WAVs are referenced by absolute file paths. Audio is not embedded in Live project state.
- Long/large WAV loading and DAW state restoration can block the caller; each WAV has a 4,000,000-frame limit.
- The waveform is a peak-envelope overview, not a zoomable sample editor; the indicator is a coarse audio-block-level view.
- The 8-voice engine releases prior voices on each trigger, so this is a crossfading slicer, not 16 independently sustaining tracks.
- Pitch changes playback rate rather than preserving duration.
- 4/4 bars are assumed. Time signature changes and sophisticated timeline discontinuities are not yet supported.
- Sample ownership with atomic shared_ptr and synchronous IO are not fully real-time safe; replace in later engine revisions.
- New state serialization must be tested with Live projects; back up projects before opening with experimental builds.

See [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) and [docs/UI_DESIGN.md](docs/UI_DESIGN.md).
