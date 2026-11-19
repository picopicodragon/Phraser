# Phraser

Windows x64 VST3 generative sample instrument for Ableton Live 12 (JUCE 8.0.8, C++17).

## Version v0.7 (experimental)

Branch: `feature/v07-waveform-edit-tempo`. Based on v0.6.1. Do not overwrite the current working VST3 until Windows CI and Live tests succeed.

### Direct waveform editing

The previous `SAMPLE nn / RANDOM WINDOWS` inspector panel has been removed. All 16 waveform lanes are editable in-place:

- **Click+drag across a waveform:** set that sample's **START FROM** and **START TO** as a normalized position interval. Green tinted area and green endpoint lines show the selected interval.
- **Shift+drag across the same waveform:** set **LENGTH MIN** and **LENGTH MAX** as normalized fractions of that WAV's total duration. A narrow ochre band beneath the waveform shows these bounds.
- These ranges are used when pressing **GENERATE**. Each generated slice chooses its own random start and duration from the associated sample's ranges. The global SIZE control still scales duration.
- Single clicking creates a zero-width start range (fixed start position) or a minimal duration range; drag for randomness.
- Clicking the sample filename opens the WAV picker. Each row's buttons sit immediately to the right of the sample name.

### Per-sample playback mode and tempo

Each WAV slot offers the following controls on its row:

- **FULL / 5ms**: play each slice until its programmed end (FULL), or begin a 5ms release on the next sequencer step (5ms).
- **SYNC / FREE**: tempo-synchronized playback speed vs unmodified sample playback speed.
- **BPM**: automatic BPM estimate from WAV onset spacing, shown in an editable text field. Click to override with any value **40–240 BPM**, or enter `0` to return to auto-detection. A `?` means no reliable BPM estimate was possible; SYNC then behaves like FREE until the user enters a manual BPM.

**Important:** SYNC changes the playback rate by `Live BPM / Sample BPM`. This changes pitch *as well as speed*, because this version uses resampling rather than independent time-stretching or key correction. The BPM detector is an approximate onset-interval algorithm; rhythmic subdivisions can cause half/double-time ambiguities. Correct the BPM manually where needed. Pitch and pitch spread controls are retained.

The 16 WAV paths, per-sample playback modes, SYNC/FREE choices, BPM overrides, waveform editing ranges, 64-step phrase and Keep A/B memories are persisted in Live project state. Old projects default to FREE and automatic BPM.

### Smaller GUI

The native JUCE panel now measures **1280×850 pixels** (down from 1460×1150). It keeps sixteen visible waveform lanes, the four-row sequence grid, twelve engine knobs, and a sixteen-ring overlap visualization.

### Previous features retained

- 16 WAV slots with sample peak waveform data
- Up to 32 independent overlapping voices
- 1–4 bars and 8 or 16 steps per bar, 8–64 total steps
- Transient-aware slice generation, Density, Breaks, Variation, Pitch, Gain, Fade, Swing
- Keep A/B memory, note-capable VST3 bus for Live (automatic transport-based sequencing)

## Windows build

```powershell
cd C:\Users\picopicoDragon\Documents\Phraser
git fetch origin
git switch -c feature/v07-waveform-edit-tempo --track origin/feature/v07-waveform-edit-tempo
cmake -S . -B build-v07 -G "Visual Studio 18 2026" -A x64
cmake --build build-v07 --config Release --target Phraser_VST3
```

The entire VST3 folder will be in `build-v07/Phraser_artefacts/Release/VST3/Phraser.vst3`. Shut down Ableton, back up the installed bundle and copy the entire folder to `C:\Program Files\Common Files\VST3\Phraser.vst3`, then rescan plugins.

## Limitations

- **Not yet Live-validated:** this branch must be checked in Ableton Live 12 on PC1 before release.
- Tempo estimator is a heuristic (not studio-grade beat tracking) and can be ambiguous on non-percussive sources.
- Synchronization alters pitch together with playback speed.
- Source WAV files remain external absolute paths, not embedded in Live project.
- WAV reading and host state restoration are synchronous; sample ownership uses shared_ptr and is not guaranteed fully hard-real-time safe.
- There is a 32-voice hard cap, with oldest-voice stealing. Simultaneous loud sounds can clip.
- Each waveform lane's narrow duration band displays the configured fraction of the original WAV duration, not a rendered audio preview.
- Still based on 4/4 meter and fixed UI sizing.

See `docs/DEVELOPMENT.md` for validation tasks.
