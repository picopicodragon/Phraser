# Phraser

Windows-only JUCE/C++ VST3 sample slicer instrument for Ableton Live 12.

Status: prototype source code; NOT compiled or tested in Ableton Live 12 yet.

## Build requirements
- Windows x64, Visual Studio 2026 C++ workload, CMake 3.22+, Git
- Internet connection to fetch JUCE during initial CMake configure

## Build (PowerShell)
```powershell
git clone https://github.com/picopicodragon/Phraser.git
cd Phraser
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release --target Phraser_VST3
```

Copy the generated Phraser.vst3 **bundle folder** from build/Phraser_artefacts/Release/VST3 to C:/Program Files/Common Files/VST3, then rescan VST3 plugins in Ableton Live 12.

## MVP
- Load mono/stereo WAV sample via the GUI
- Randomize a 16-step slice pattern
- Playback synced to host tempo and PPQ position (sixteenth-note steps)
- Density, Slice length and Gain controls
- Stereo instrument output; no video functionality

## Known limitations
- WAV only; no pitch shift, time stretch, transient detection, FX or multilayer support yet.
- Host must be playing and provide tempo/PPQ position.
- Sample data and generated pattern are not yet stored in DAW project state; reload WAV after restoring a session.
- No Windows compilation or Ableton Live 12 runtime verification has been performed.

## Roadmap
1. Confirm build and Live 12 integration; fix observed issues.
2. Implement sample/pattern state persistence, transient cuts and music-aware generation.
3. Expand to multilayer Keep/Hold, pitch shifting and export.

See docs/DEVELOPMENT.md for verification steps.