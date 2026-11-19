# v0.8 experimental acceptance / regression tests

## Background

v0.7.1 fixed an out-of-bounds BPM histogram write that could crash Ableton Live when loading WAV. v0.8 inherits that fix and adds independent tuning from tempo matching.

## PC1 build

```powershell
cd C:\Users\picopicoDragon\Documents\Phraser
git fetch origin
git switch -c feature/v08-independent-pitch-stretch --track origin/feature/v08-independent-pitch-stretch
cmake -S . -B build-v08 -G "Visual Studio 18 2026" -A x64
cmake --build build-v08 --config Release --target Phraser_VST3
```

Backup the prior installed VST3 and work only on a disposable Live set until tests pass.

## Acceptance checklist

- [ ] Windows CI and VS2026 compile/link Phraser_VST3 with no errors.
- [ ] Import short percussion loop and longer rhythmic WAV without Live crashing (regression for v0.7.1).
- [ ] All 16 rows show an editable TUNE field adjacent to BPM.
- [ ] TUNE=0, WAV BPM=120, Live BPM=120, SYNC plays at approximately original pitch and duration.
- [ ] TUNE=0, WAV BPM=120, Live BPM=90, SYNC produces approximately original pitch but longer duration.
- [ ] TUNE=0, WAV BPM=120, Live BPM=160, SYNC produces approximately original pitch but shorter duration.
- [ ] With SYNC enabled, change Live BPM while a long slice is sounding: speed/duration follows and pitch stays close to the chosen TUNE.
- [ ] TUNE=+12 sounds approximately one octave above TUNE=0 at the same Live BPM. TUNE=-12 sounds approximately one octave lower.
- [ ] FREE ignores tempo changes in source playback speed. TUNE still changes pitch without intentionally changing sample slice duration.
- [ ] Two WAV slots with different pitch tuning play independently and their voices overlap as expected.
- [ ] Global pitch and step pitch spread combine with per-slot TUNE with a maximum total of ±24 semitones.
- [ ] FULL and 5ms release mode still behave independently per slot.
- [ ] The orbit's predicted arc duration changes with BPM matching but not the selected tuning.
- [ ] Save/reopen Live project restores per-WAV tuning, BPM, SYNC/FREE, playback mode, wave ranges and phrase state.
- [ ] Older v0.7.1 Live project state without pitchSemitones defaults to TUNE=0.
- [ ] Stress test dense 32-voice playback at 44.1/48 kHz, host stop/start and rapid BPM changes.
- [ ] Listen for granular phasing, flams, flutter and attack smearing in drums, melodic loops and sustained pads. Report source WAV, BPM settings, tune and host sample rate if audible.

## Algorithm quality limits

Current implementation is two-grain overlap-add with 30ms hop and a bounded waveform-similarity search at grain boundaries. It is WSOLA-inspired, but not a complete pitch-synchronous WSOLA, transient-aware vocoder, or professional formant-preserving stretcher. It approximates independent tempo and pitch; complex signals and extreme ratios may sound granular. Avoid describing it as transparent high-fidelity time stretching. Dedicated DSP tests and host automation tests remain future work.
