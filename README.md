# Phraser

Windows x64 VST3 instrument for Ableton Live 12, developed with C++17, JUCE 8 and CMake.

**main** is the last user-tested v0.2 prototype. The `feature/phrase-engine-v03` branch contains **experimental v0.3** source, not yet compiled or verified in Live 12.

## New in experimental v0.3

- **Transient Snap**: lightweight offline energy-rise detection when loading WAV; generated slice positions optionally snap to detected onsets.
- **Variation**: adjusts motif repetition and switching of random sample positions across a 16-step phrase.
- **8 voice pool**: a new slice starts as the earlier voice releases, allowing short overlaps instead of abrupt one-voice cuts.
- **Short crossfade**: previous voices release for about 5 ms at each new sixteenth-note event.
- All v0.2 features retained: four sample slots, 16 steps, per-step enable and LOCK, Density, Breaks, Size, Swing, Gain, Fade, DAW state with external WAV file paths.

## Setup

Requires Windows x64, Visual Studio 2026 with the C++ desktop workload, CMake >= 3.22, Git, and Ableton Live 12.

```powershell
git clone https://github.com/picopicodragon/Phraser.git
cd Phraser
git switch feature/phrase-engine-v03
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release --target Phraser_VST3
```

Output: `build/Phraser_artefacts/Release/VST3/Phraser.vst3` (bundle directory).

**Do not overwrite the previously working installation until the new binary compiles.** Exit Ableton completely; back up `C:\Program Files\Common Files\VST3\Phraser.vst3`; copy in the new bundle using an elevated PowerShell session; start Live and rescan.

## Controls

- Four slot buttons load WAVs.
- GENERATE randomizes unlocked steps. Density and Breaks shape rests; Size sets slice duration.
- SNAP controls how often randomly chosen positions are moved to nearby detected attacks.
- VARIATION reduces motif repetition at high values; lower values repeat more.
- SWING offsets alternate 16th-note steps; GAIN and FADE affect playback.
- Click a step to mute/unmute, click LOCK beneath it to protect its contents on regenerate.

## Current limitations

- **Not yet verified**: Windows compilation and Live 12 integration for v0.3.
- Onset detection is an energy-rise approximation, not a spectral-flux or machine-learning transient detector.
- No pitch shift, time stretch, attack classification, wav export or multibus routing yet.
- Up to 8 overlapping voices; dense rapid retriggering may steal a voice, and no true constant-power crossfade.
- Audio is saved as external absolute WAV paths. Missing files remain silent.
- WAV decoding on state restore may block the host; realtime-safe ownership and async file IO remain future work.
- Pattern is one 4/4 bar.
- Controls are normalized float parameters and are saved by the DAW; regenerate after changing Density, Breaks, Size, Snap or Variation.

See [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) for PC1 verification tasks.
