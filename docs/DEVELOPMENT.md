# v0.6.1 release-mode acceptance tests

Two options per WAV slot:
- FULL SLICE LENGTH (default): full independent slice sustain.
- 5ms FADE AT NEXT STEP: fade that slot's existing voices for 5ms when sequencer advances, including on rest steps.

## Windows build

```powershell
cd C:\Users\picopicoDragon\Documents\Phraser
git fetch origin
git switch -c feature/v061-per-slot-playback-mode --track origin/feature/v061-per-slot-playback-mode
cmake -S . -B build-v061 -G "Visual Studio 18 2026" -A x64
cmake --build build-v061 --config Release --target Phraser_VST3
```

## Checklist

- [ ] Windows VST3 compilation completes without errors.
- [ ] Live 12 loads Phraser and shows playback mode button in selected WAV inspector.
- [ ] Every sample slot has its own mode. Switching slot restores its own selection.
- [ ] Mode button changes only future triggers and does not alter sample random-start or random-length controls.
- [ ] FULL LENGTH notes persist across subsequent steps and rests, overlapping other sample voices.
- [ ] 5ms FADE notes end smoothly when a subsequent step arrives, including a rest.
- [ ] Two tracks with different modes behave differently within the same phrase.
- [ ] Samples that finish naturally before next step remain silent as expected.
- [ ] WAV buttons show F or L for the configured mode; circular preview updates.
- [ ] Live project save/reload preserves per-slot modes.
- [ ] v0.6 projects without the new attribute load into FULL LENGTH mode.
- [ ] Transport stop clears active voices and no stuck audio remains.
- [ ] Playback remains stable with 32 simultaneous voices; oldest voice is stolen at cap.

Known limitations: 32-voice steal is abrupt, peak sums can clip, original audio remains external, disk loading is synchronous, and pointer reclamation is not hard realtime safe.
