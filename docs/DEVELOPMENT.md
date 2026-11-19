# v0.6 build and QA gate

The v0.6 branch adds 32 independent sample voices and a bright circular overlap visualization to the 16-WAV v0.5 workstation. Merge only after testing; v0.5/v0.4 branches remain separate.

## Windows build

```powershell
cd C:\Users\picopicoDragon\Documents\Phraser
git fetch origin
git switch -c feature/v06-polyphonic-light-orbit --track origin/feature/v06-polyphonic-light-orbit
cmake -S . -B build-v06 -G "Visual Studio 18 2026" -A x64
cmake --build build-v06 --config Release --target Phraser_VST3
```

## Acceptance checklist

- [ ] Build succeeds with MSVC, no JUCE/MSVC C4819 regression.
- [ ] Ableton Live 12 loads instrument, and no startup error occurs.
- [ ] GUI is visibly brighter, waveforms legible, original color identity.
- [ ] All 16 WAV files still load and display peaks.
- [ ] 16-ring overlap circle renders 16 distinguishable tracks and arc spans.
- [ ] Current voice count rises above 1 when long slices overlap at fast tempo.
- [ ] The same WAV can retrigger while the previous slice still sounds.
- [ ] A rest step does **not** cut the previous sample.
- [ ] Voice ends at its own slice boundary with envelope; no sudden cutoff on next normal step.
- [ ] With 32 active voices, the oldest voice is stolen without crash (may click).
- [ ] The orbit highlights every sample slot that currently has one or more active voices.
- [ ] Waveform playback markers remain functional; sample timing and step grid still editable.
- [ ] Saved Ableton project restores all WAV slots, pattern, range settings and Keep A/B.
- [ ] Tempo 80→160 with long slices, transport stop/restart, and loop seeks do not crash.
- [ ] Output levels stay controllable. Layering can exceed 0 dBFS, so test gain reduction.
- [ ] Monitor CPU at 44.1k/48k with 16 samples and many overlapping voices.

## Known shortcomings

- Hard voice cap 32; oldest-voice stealing is abrupt.
- 16-ring circles are dense at 1460×1150, verify DPI and usability.
- UI orbit is a predictive step visualization, not a precise oscilloscope of all active voices.
- Shared_ptr ownership and synchronous disk loads still need a real-time-safe refactor.
- No MIDI-note-triggered instruments yet; voices are sequencer-generated.
