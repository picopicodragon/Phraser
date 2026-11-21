# Phraser v1.0: regression and acceptance

Branch: `feature/v10-key-mixer-orbit-mutes`, based on v0.9.1.

## Windows build

```powershell
cd C:\Users\picopicoDragon\Documents\Phraser
git fetch origin
git switch -c feature/v10-key-mixer-orbit-mutes --track origin/feature/v10-key-mixer-orbit-mutes
cmake -S . -B build-v10 -G "Visual Studio 18 2026" -A x64
cmake --build build-v10 --config Release --target Phraser_VST3
```

## Must-pass PC1 checks

- [ ] Windows VST3 builds without compile/link errors.
- [ ] In Ableton Live 12, 16 sample lanes all fit in 1280x850 with MUTE and VOLUME+meter.
- [ ] Click WAV name for chooser; drag WAV from Explorer into NAME column; drag sample onto grid.
- [ ] The sample’s detected key appears when reliable. Set PREFERENCE KEY to C then G and verify sample transposition.
- [ ] For a known A 440Hz sine, force SOURCE ROOT=A using sample-name right-click, PREFERENCE KEY=C, TUNE=0 and check output near C frequency (approx +3 semitones); then PREFERENCE KEY=A should return to 440Hz.
- [ ] For non-tonal drums without detected root, AUTO leaves pitch unchanged; manual root override works.
- [ ] Changing preference while a long voice sustains updates its pitch while retaining the current tempo.
- [ ] Every SAMPLE has its own mute. Muted slot silences already playing voice and prevents new triggers.
- [ ] Track T1..T4 mute buttons are visible only for the current number of tracks; mutes silence that track's playback.
- [ ] Every sample volume slider affects just that WAV; 0% silent, 100% unity, 150% louder (allowing clipping).
- [ ] Green peak meter overlay rises on sample playback and decays/falls with silence, independent per WAV.
- [ ] Grid assigned sample colors are all distinct, even for adjacent sample slots; inactive but loaded assignments show color.
- [ ] Orbit ring 1..16 colors correspond exactly to sample colors; arcs show event spans and active voice count.
- [ ] BARS=1 shows one labeled bar sector; BARS=2 shows 2; BARS=3 shows 3; BARS=4 shows 4.
- [ ] BARS 1/2/3/4 and STEPS 8/16 still give correct timing, grid layout and parallel tracks.
- [ ] Save/reopen Live project restores 16 sample gains, source mutes, track mutes, source root overrides, reference key and old settings.
- [ ] Old v0.9.1 projects default to all unmuted, 100% volume and AUTO source key without errors.
- [ ] Live transport start/stop resets meters/voices; no stale activity.
- [ ] Test 32-voice overlap and rapid mute/key changes for crashes, clicks, CPU spikes and plugin window stability.

Known unresolved issues: pitch detection can be ambiguous, granular pitch correction can sound rough, file loading remains synchronous, sample handoff is not strictly hard realtime safe, and mix gain can clip.

## v1.0.1 regression QA

- [ ] PHRASE ENGINE 12 rotary value fields have clearly readable text in white/cream boxes.
- [ ] KEEP A, KEEP B, RECALL A, RECALL B have visible dark text in both normal and hover states.
- [ ] Each volume control shows volume position as a marker and percentage, with a live green level overlay **within the same bar**.
- [ ] Per-sample peak overlay responds independently to audio from that slot and recedes when silent.
- [ ] Enabled step cells retain sample-specific colors and display two-digit **sample slot numbers**, not step column numbers.
- [ ] Silent steps have almost-white backgrounds yet their assigned sample's number and thin color marker remain readable.
- [ ] At BARS=4/STEPS=16 (64 columns), sample numbers fit within the narrowest cells; no spill outside cells.
- [ ] At 8/16 subdivisions and all BARS modes, step labels and drag/drop hitboxes remain aligned.
- [ ] Existing source/key tuning, sample/track mute, DAW save/restore and WAV loading remain functional.
- [ ] Test on Windows with 100% and 150% display scaling; verify plugin UI legibility.
