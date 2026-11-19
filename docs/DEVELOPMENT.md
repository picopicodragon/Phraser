# Windows / Ableton Live 12 validation

## Environment
- Visual Studio 2022, Windows SDK, x64 C++ workload
- CMake >= 3.22 and Git in PATH
- JUCE is fetched from GitHub during CMake configure (pinned tag 8.0.8)

## Compile
From a Visual Studio Developer PowerShell:
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target Phraser_VST3
```

Check the artifact under build/Phraser_artefacts/Release/VST3/.

## Live acceptance checklist
- [ ] VST3 loads on a new MIDI track and opens the editor.
- [ ] Load WAV accepts short mono and stereo PCM WAV files.
- [ ] While transport is running the sample is sliced into 16th-note events.
- [ ] Changing BPM changes trigger rate without changing event count.
- [ ] Density=0 produces silence and Density=1 produces full event density.
- [ ] Randomize generates a different arrangement.
- [ ] Loop boundaries and transport restart are stable.
- [ ] Changing Gain does not cause clipping unexpectedly.
- [ ] Closing and reopening the plugin does not crash Live.
- [ ] Automation and host parameter reload work.
- [ ] No clicks with discontinuous slices beyond expected prototype limitations.

Known limits: sample and pattern are not persisted, no interpolation for sample-rate mismatch, no pitch/time stretching. Source loading is UI-thread only. This is unverified source code, not a validated release.
