# Development / Windows Live 12 acceptance tests

## Current stage
v0.2 experimental: four WAV slots, 16 steps, step locks, swing/rests, envelope and session-state references. No Windows build confirmation yet.

## Build on PC1
```powershell
cd C:\Users\picopicoDragon\Documents\Phraser
git pull origin main
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release --target Phraser_VST3
```

For a safe rollback, copy the currently working VST3 directory elsewhere **before installing** the new build.

## Functional checks
- [ ] Compiles on Visual Studio 2026 without errors, including the GUI and processor.
- [ ] Ableton Live 12 opens plugin on a MIDI track (event input bus exists).
- [ ] All four WAV buttons open the chooser and show filenames.
- [ ] Loaded mono/stereo WAV files both play; different slots sound different.
- [ ] GENERATE produces different combinations of slices/rests.
- [ ] Density 0 + Breaks 0 -> empty pattern after Generate.
- [ ] Density 1 + Breaks 0 -> 16 active steps after Generate.
- [ ] Step click mutes/unmutes while playback continues.
- [ ] Lock keeps a step unchanged after Generate, including slot/start/length.
- [ ] Swing 0 plays straight sixteenths; increasing Swing delays odd steps.
- [ ] Fade reduces hard edges without causing unexpected silence.
- [ ] BPM change retains sixteenth-note synchronization.
- [ ] Save/reopen the Live project restores parameters, step state and WAV paths.
- [ ] Missing WAV after reopening does not crash Live.
- [ ] Closing/reopening editor while transport runs does not crash Live.

## Known technical debt
- Disk loading in setStateInformation is synchronous, which may run on a host-sensitive thread. Move loading to a cancellable worker in a future version.
- Shared_ptr snapshot refcount operations in processBlock are thread-safe but not guaranteed hard real-time safe. Replace with realtime-safe ownership when optimizing.
- A single active voice means no overlapping slices. Add polyphonic voice management and crossfades.
- The step engine assumes 4/4, one bar, and derives PPQ at the start of each process block. Host time signature and advanced transport discontinuities need explicit support.
- Pattern edit operations may conflict if GUI calls race with host state restore. Add a synchronized control-thread mutation queue.
- No automated audio regression tests; Windows-host validation remains required.
