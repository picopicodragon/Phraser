# Phraser v0.4 verification / acceptance gate

This branch is **experimental**. v0.3 source is the base. Do not merge until v0.4 compiles on PC1 and passes Live 12 smoke tests.

## Build

```powershell
cd C:\Users\picopicoDragon\Documents\Phraser
git fetch origin
git switch -c feature/v04-orbit-ui-pitch --track origin/feature/v04-orbit-ui-pitch
cmake -S . -B build-v04 -G "Visual Studio 18 2026" -A x64
cmake --build build-v04 --config Release --target Phraser_VST3
```

Use `git switch feature/v04-orbit-ui-pitch` if the local branch exists.

## Checklist

- [ ] VS2026 Release VST3 builds, with no errors or MSVC C4819 warning.
- [ ] Ableton Live 12 recognizes the VST3 instrument and editor opens.
- [ ] Orbit shows four independent ring colours and current active step.
- [ ] Left-click mutes/unmutes one step; Shift+click changes step's source ring.
- [ ] Right-click locks a step and the marked step survives NEW PHRASE.
- [ ] 4 WAV buttons load distinct samples.
- [ ] Ten knobs attach to host parameters; automation values persist.
- [ ] Pitch +12 raises the sample one octave and doubles its play rate; -12 lowers it.
- [ ] Pitch Spread generates different note transpositions on unlocked steps, after regenerate.
- [ ] SAVE/RECALL A/B restores expected phrase, including step-level pitch.
- [ ] Ableton save and reopen restores current phrase, Keep A/B, pitch, sample paths.
- [ ] Playback remains stable at 44.1k and 48k sample rates and when tempo changes.
- [ ] Changing project/closing editor while playing does not crash Live.
- [ ] Compare GUI at Windows 100% and 150% display scaling for label clipping.
- [ ] Measure CPU and peak level on dense overlapping samples.

## Technical debt / prioritization

1. Replace synchronous sample disk IO and atomic-shared_ptr ownership with a realtime-safe sample bank exchange and background loader.
2. Distinguish tempo-independent pitch shift from simple playback rate and introduce HQ resampling/anti-aliasing.
3. Add 4/4 meter-independent 1–4 bar phrases, MIDI note gate and clip export.
4. Test VST3 compatibility using pluginval/Steinberg tests and audio regression benchmarks.
5. Add GUI resize support, focus accessibility and advanced per-step editing.
