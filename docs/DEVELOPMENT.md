# v0.7 QA checklist / PC1

## Build commands

```powershell
cd C:\Users\picopicoDragon\Documents\Phraser
git fetch origin
git switch -c feature/v07-waveform-edit-tempo --track origin/feature/v07-waveform-edit-tempo
cmake -S . -B build-v07 -G "Visual Studio 18 2026" -A x64
cmake --build build-v07 --config Release --target Phraser_VST3
```

## Must-pass validation

- [ ] Windows release builds without compilation errors or C4819 warnings.
- [ ] Ableton Live 12 loads v0.7 and panel is 1280×850 with 16 lanes and circle.
- [ ] No separate SAMPLE / RANDOM WINDOWS section remains.
- [ ] Dragging WAV horizontal range changes START FROM/TO independently for the selected WAV.
- [ ] Shift+drag changes LENGTH MIN/MAX and an ochre duration bar appears on the waveform.
- [ ] Generate honors edited starts and independently randomized lengths.
- [ ] FULL/5ms toggles are visible immediately next to every WAV name, independent per slot.
- [ ] SYNC/FREE toggles are visible next to each WAV and toggle only their slot.
- [ ] Loading rhythmic WAV reports plausible detected BPM in each slot's editable field.
- [ ] A sustained/ambiguous sample shows ? instead of inventing a BPM; manual BPM override works.
- [ ] Override 120 BPM / Live 120 BPM behaves like FREE. Live 160 BPM with 120 BPM source plays 4/3 faster in SYNC.
- [ ] FREE playback speed is unchanged by Live BPM updates; it remains transport-triggered at the correct steps.
- [ ] BPM, sync flags, random range edits, samples, playback end modes and patterns restore after saving and reopening a Live project.
- [ ] v0.6.1 state without new attributes defaults to SYNC OFF and AUTO BPM.
- [ ] 32 overlapping voices, long slices and 5ms endings remain stable at 44.1/48 kHz.
- [ ] CPU, memory and panel labels are usable at Windows 100% / 150% display scaling.
- [ ] Stop/start, Live loop and tempo changes do not crash or leave stuck notes.

## Known constraints

Onset-spacing BPM detection can select half/double time. Resampling changes pitch. File loading is synchronous, shared_ptr memory reclamation is not hard realtime safe, hard voice stealing can click, and overall output may clip without a limiter. Keep a known-good VST3 backup.
