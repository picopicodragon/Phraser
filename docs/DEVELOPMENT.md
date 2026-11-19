# v0.3 experimental verification on PC1

Keep the last known-good v0.2 VST3 backed up; do not merge experimental v0.3 until built and checked in Ableton Live 12.

## Build

```powershell
cd C:\Users\picopicoDragon\Documents\Phraser
git fetch origin
git switch feature/phrase-engine-v03
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release --target Phraser_VST3
```

Build directory may be shared with v0.2 because CMake will regenerate; if configuration becomes invalid use a fresh `build-v03` directory instead.

## Acceptance checklist

- [ ] Windows MSVC compiles without errors or C4819 warnings.
- [ ] VST3 is recognized as a MIDI-capable instrument by Ableton Live 12.
- [ ] GUI shows 8 knobs, 4 samples, 16 step toggles and 16 locks.
- [ ] Short percussive WAV reports more than one onset mark.
- [ ] Sustained/quiet WAV safely reports zero or a small number of additional onset marks.
- [ ] SNAP=1 and GENERATE tend to choose attacks; SNAP=0 restores free selection.
- [ ] VARIATION=0 yields more repeat motifs; VARIATION=1 yields no forced repeats.
- [ ] All four WAV slots can produce different timbres.
- [ ] Repeated short slices avoid obvious hard-edge clicks; measure output at boundaries.
- [ ] Repeated fast steps never crash or produce nonfinite samples.
- [ ] On/off clicks and LOCK preserve existing behavior.
- [ ] Save and reopen Live project, reload referenced WAV paths and restore phrase and parameters.
- [ ] CPU usage remains acceptable while playback loops and GUI is open/closed.
- [ ] Stop/restart and loop jumps do not crash or retain stale voices.

## Known architectural debt

1. Shared-pointer acquisition/destruction on the audio thread may deallocate memory, so hard realtime safety is **not guaranteed**. Replace with deferred reclamation/immutable banks in a later revision.
2. Onset detection is offline but file loading and state restoration may block callers. Move to worker thread, communicate loaded samples through a realtime-safe handoff.
3. Voice stealing occurs if all eight voices are occupied. Add configurable polyphony and limiting when needed.
4. Host discontinuity detection still uses step index; advanced host loop tempo-map behavior needs tests.
5. No audio-output snapshots, unit tests or automated VST3 pluginval checks yet.
