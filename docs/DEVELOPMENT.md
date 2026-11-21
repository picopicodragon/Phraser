# v0.9 QA / release gate

Branch: `feature/v09-drag-key-multitrack`.

## Build

```powershell
cd C:\Users\picopicoDragon\Documents\Phraser
git fetch origin
git switch -c feature/v09-drag-key-multitrack --track origin/feature/v09-drag-key-multitrack
cmake -S . -B build-v09 -G "Visual Studio 18 2026" -A x64
cmake --build build-v09 --config Release --target Phraser_VST3
```

Keep previous working plugin bundle backed up and use a throwaway Live set until all acceptance criteria pass.

## Acceptance checklist

- [ ] Windows CMake, Release VST3 build and bundle upload succeed without C4819 regression.
- [ ] Ableton Live 12 opens v0.9 without warnings or host crashes; v0.7.1 WAV-import fix still works.
- [ ] Drag WAV from Explorer onto sample-name row 1, row 8, row 16; waveforms refresh and sound works.
- [ ] Drop multiple WAVs onto row 5 and confirm they fill consecutive slots without exceeding 16.
- [ ] Clicking sample name still opens file picker; dragging it to the grid does not also open the picker.
- [ ] Drag sample name onto a step cell in each BARS configuration; cell receives correct slot and enables.
- [ ] Right-click grid offers all 16 WAV slots and toggle/lock actions; selects the exact intended cell.
- [ ] Left-click mute/unmute, Shift+click assign selected, and LOCK survive regenerate.
- [ ] REFERENCE KEY dropdown C to B saves/restores with the Live project.
- [ ] For a pure tonal A=440Hz WAV, reference C gives about +3 semitone auto offset; reference A gives 0.
- [ ] For a steady C=261.6Hz WAV, reference D gives about +2 semitone offset.
- [ ] Drum noise and pitch-ambiguous sources show unknown and remain untuned by automatic key offset.
- [ ] Per-WAV manual TUNE combines with auto offset and playback remains tempo-independent in SYNC mode.
- [ ] BARS=1: 4 tracks × 16 columns each; place four simultaneous events at column one and hear them overlap.
- [ ] BARS=2: 2 tracks × 32 columns each; trigger both at step 1 and 32.
- [ ] BARS=3: 1 track × 48 columns; playback loops after 3 bars.
- [ ] BARS=4: 1 track × 64 columns; playback loops after 4 bars.
- [ ] Grid visuals, mouse hit boxes, and dragged assignments use identical mapping at all four settings.
- [ ] Circular overlap display includes arcs from every active parallel track.
- [ ] Save/reopen Live project restores WAV paths, sample modes, pitches, key reference, Keep A/B and phrase events.
- [ ] Live tempo automation, 32-voice polyphony, transport stop, loop and rapid BARS changes do not crash.

## Known risks / follow-up

- Automatic root detection is a simple spectral pitch-class heuristic and should never be marketed as reliable full musical-key detection for complex 2MIX.
- GUI file dropping, waveform analysis, and state restoration are synchronous.
- Rapid BARS changes will reinterpret existing event cells. Add migration/independent configurations if preserving all arrangements becomes necessary.
- CPU use from 32-voice resynthesis, 16 waveform lanes, and live circular rendering must be measured.
