# Phraser

Independent Windows x64 VST3 generative sample instrument for Ableton Live 12, built with JUCE 8.0.8 and C++17.

## v0.9.1 experimental

Development branch: `feature/v091-restore-steps-control`. Base is v0.9, which itself is based on v0.8 pitch-independent time-stretch implementation. Existing working builds remain on their original branches.

### 1. WAV drag-and-drop

Drag one or more WAV files from Windows Explorer onto the **SAMPLE NAME** column of any row. A single WAV loads into that sample slot; multiple WAVs fill consecutive slots up to number 16. Click the sample name to open the normal file picker, as before. The import is still synchronous and subject to the existing per-WAV size limit.

### 2. Reference note and automatic transposition

A **REFERENCE KEY** drop-down in the header accepts C, C#, D, D#, E, F, F#, G, G#, A, A#, B. This is a **pitch class/reference root**, not a major/minor musical-key classification.

When a WAV is loaded, an offline 4096-point FFT analyzes its spectral peaks across several time windows. For **confidently monophonic/tonal** material, it estimates a dominant pitch class. Playback then applies the shortest transposition within ±6 semitones from the detected pitch class to the selected reference pitch class. This auto-tune offset is combined with the source's editable TUNE (±12 semitones), the global PITCH, and the phrase event's pitch offset; the overall sum is limited to ±24 semitones. The v0.8 pitch-independent granular engine preserves this tuning when Live BPM changes.

For drums, noise, chords, or ambiguous material, the detector may return **unknown**, in which case the automatic transpose is zero rather than forcing a guessed pitch. The WAV's detected note appears beside its shortened name (when known), and the sample button tooltip shows its current automatic semitone offset. The reference key is a DAW parameter, so it is stored with Live projects and affects newly triggered voices.

**Important:** This is a conservative dominant-pitch detector, not professional musical-key detection or formant-preserving pitch correction. Complex/polyphonic recordings may be misidentified; users can still adjust per-WAV TUNE manually. No automatic major/minor key or chord analysis is claimed.

### 3. Phrase-cell sample assignment

- **Drag a sample name** from the SAMPLE list onto a phrase grid cell to assign it to that cell and enable it.
- **Right-click any phrase cell** to open a context menu listing all 16 WAV slots, including an option to toggle the step or toggle its LOCK state.
- Normal **left-click** toggles step mute; **Shift+click** assigns the last-selected WAV. These functions are preserved.
- Loading a WAV into a previously assigned slot changes the timbre of all the cells using that slot, without changing their arrangements.

### 4. BARS and STEPS: parallel-track arrangement

Each bar uses **8 or 16 steps**, selected by the restored **STEPS** control beside BARS. The host tempo clock advances by eighth notes at 8 steps/bar or sixteenth notes at 16 steps/bar. BARS selects the length of each track and the number of **simultaneously running tracks**, within the same 64-cell pattern budget:

| BARS | STEPS=8 (per track) | STEPS=16 (per track) | Parallel tracks |
| --- | --- | --- | --- |
| 1 | 8 | 16 | 4 |
| 2 | 16 | 32 | 2 |
| 3 | 24 | 48 | 1 |
| 4 | 32 | 64 | 1 |

Within every bar, all enabled tracks trigger their corresponding cells **at the same transport position**. For one bar at STEPS=16, four separate 16-step rows play in parallel. For two bars at STEPS=16, two 32-step rows play in parallel. Three and four bars use one longer row respectively. STEPS=8 halves the number of columns and uses eighth-note timing. Grid cells are indexed by `row * stepsPerTrack + step`.

The existing `division` parameter is **active again** as STEPS (8 or 16), beside BARS; old project values remain compatible. BARS changes the interpretation of the 64 stored pattern events, so **a pattern edited at one BARS setting may sound differently after changing BARS**. Use Keep A/B or separate Live sets to preserve alternate configurations.

### Preserved features

- 16 WAV slots and waveform peaks with dragging to edit START FROM/TO and LENGTH MIN/MAX.
- Per-WAV FULL/5ms, SYNC/FREE, editable BPM and TUNE.
- Source onset-based sample tempo estimate, pitch-independent resynthesis, 32-voice overlap.
- Random generation, density, breaks, variation, swing, transient snap, pitch spread, gain/fade.
- Phrase Keep A/B snapshots, project-state persistence and circular overlap visualization.

## Windows build

```powershell
cd C:\Users\picopicoDragon\Documents\Phraser
git fetch origin
git switch -c feature/v091-restore-steps-control --track origin/feature/v091-restore-steps-control
cmake -S . -B build-v091 -G "Visual Studio 18 2026" -A x64
cmake --build build-v091 --config Release --target Phraser_VST3
```

VST3 bundle: `build-v091/Phraser_artefacts/Release/VST3/Phraser.vst3`. Quit Live and back up the previous installed VST3 bundle before replacing `C:\Program Files\Common Files\VST3\Phraser.vst3`.

## Known limitations

This is an **experimental** branch. Build success does not establish Ableton Live 12 host reliability.

- Source pitch estimation is strongest on steady, monophonic, pitched signals; not a robust full-song harmonic key estimator.
- Samples with ambiguous pitch are not automatically tuned. Automatic transpose uses the nearest ±6 semitone distance between root pitch classes, not a musical mode.
- Existing two-grain WSOLA-inspired tempo/pitch processing can generate audible modulation on some material.
- Files are imported and analyzed synchronously, and audio-thread shared_ptr retirement is not strictly hard-real-time safe.
- Max 32 simultaneous voices; oldest-voice stealing is abrupt and simultaneous sources can clip.
- WAVs remain external, referenced by absolute paths. Existing file size restrictions apply.
- 64 stored cells are reinterpreted when BARS changes, rather than maintaining separate arrangement snapshots for each BARS value.
- Fixed 1280×850 GUI. At 48/64 steps in a single track the cells become narrow; hover/context menu and drag are preferable to reading tiny step labels.

See [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) for Windows/Live QA.
