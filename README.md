# Phraser

Windows-only VST3 instrument for Ableton Live 12. Written in C++17 with JUCE 8.

**Status: experimental v0.2 source; the latest changes have NOT yet been compiled or tested on Windows.** The earlier v0.1 prototype built and was tested for WAV loading/playback on Live 12.

## Features in the new experimental source
- Four independently loadable mono/stereo WAV slots
- 16-step bar-synced phrase generator, multiple samples chosen across steps
- Density, Breaks, Size, Swing, Gain and Fade parameters
- Click a step number to enable/mute that event
- Click LOCK under any step to protect it from future GENERATE operations
- Fade-in/out envelope at slice boundaries; interpolated sample playback across sample rates
- Save the sixteen steps, parameters, and **WAV file paths** into DAW state. WAV audio is referenced externally, not embedded.
- MIDI input event bus for host instrument compatibility, but transport-sync playback (notes not required)
- No video

## Windows requirements
- Visual Studio 2026 Community with C++ desktop workload (or VS 2022 with the matching generator)
- CMake >= 3.22; Git; x64 Windows; Ableton Live 12
- First CMake configuration fetches JUCE 8.0.8

## Build (PowerShell, VS 2026)
```powershell
git clone https://github.com/picopicodragon/Phraser.git
cd Phraser
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release --target Phraser_VST3
```

Built output: `build/Phraser_artefacts/Release/VST3/Phraser.vst3` (a directory/bundle). Close Live and copy the **entire** bundle to `C:\Program Files\Common Files\VST3\Phraser.vst3`; launch Live and rescan VST3.

## Operation
1. Insert Phraser on a **MIDI track**, then choose up to four WAVs.
2. Press **GENERATE**. Each enabled step chooses from the loaded sample slots.
3. Start Live's transport; the phrase plays on sixteenth-note steps.
4. Adjust Density/Breaks/Size, then GENERATE again to apply them to new steps. Swing/Gain/Fade act during playback.
5. Click a step to mute/unmute it. Click LOCK to keep the event when regenerating.
6. Save Live project; sample paths and pattern are stored. Keep the WAVs at those paths.

## Important limitations
- Only WAV, no transient detection, high-quality time stretching or pitch correction yet.
- Single active slice voice; steps cut off the preceding slice.
- Very short per-slice fades reduce clicks but are not full crossfades.
- Sample loading is synchronous on UI thread and state restoration; very large files may block. Do not use huge WAVs yet.
- Saved audio is referenced by absolute path. Missing/moved files are silent; there is no relocation dialog yet.
- Generator is rhythmically random, not yet music-aware; each pattern is one bar long in 4/4.
- GitHub Actions uses VS 2022 for additional CI coverage; local instructions use VS 2026.
- v0.2 remains **unverified** until a Windows build and Live 12 check succeed.

See [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) for acceptance tests and next milestones.
