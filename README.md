# Phraser

Windows x64 VST3 phrase sampler for Ableton Live 12, written with C++17 and JUCE 8.0.8.

## v0.8 experimental: pitch-independent sample tempo sync

**Branch**: `feature/v08-independent-pitch-stretch`, based on the v0.7.1 WAV-import crash hotfix. **The working installed version remains untouched**. Windows CI compilation and Live 12 audio-quality tests are required.

### What changed

- **Per-WAV TUNE** field directly to the right of each sample's editable BPM. Enter **-12 to +12 semitones**, with `0` for original pitch.
- **SYNC** changes sample duration to match `Live BPM / detected or manually-entered sample BPM` **without multiplying the audio read rate by that ratio**. Source pitch is held by a two-grain overlap-add resynthesis engine. A Live tempo change during playback updates the voice's timeline advancement but does not alter the voice's grain pitch read rate.
- The global `PITCH` knob and phrase `PITCH SPREAD` remain in place. Actual pitch = global pitch + per-WAV TUNE + step pitch, limited to ±24 semitones.
- For FREE, the timeline advances at the sample's original rate; TUNE alters pitch without intentionally shortening the programmed slice's duration.
- 32 independent voices, 16 WAV waveform lanes, per-sample FULL/5ms and SYNC/FREE switches, direct waveform range edits, 64-step phrase grid, circular overlap view and Keep A/B are preserved.
- Per-sample tuning is saved in the Live project (`SampleSlot.pitchSemitones`). Old sessions default to 0 semitones. The generated phrase and previous sample path references remain compatible.

### Implementation

The v0.7 SYNC mode performed naive playback-rate resampling, which transposed the sample as Live BPM changed. v0.8 separates (a) the **source timeline** at normal or tempo-matched speed from (b) **grain-local playback** at the selected musical pitch. It crossfades two short, overlapping grains (nominal 60 ms each with 30 ms hop) using smoothstep weights. The voice's fixed-size state requires no new large buffers or allocations in its per-sample render loop.

This is an **initial, lightweight time-domain overlap-add (OLA) implementation**, not a professional formant-preserving WSOLA/phase-vocoder engine. It can produce audible phasing, graininess, transient smearing or modulation at strong tempo/pitch ratios, especially on complex or sustained material. Tempo estimation can still be inaccurate and half/double-time ambiguous; correct each WAV's editable BPM manually. A more advanced transient-aware stretching engine remains a future quality milestone.

### Controls

Each sample lane: `WAV filename | FULL/5ms | SYNC/FREE | BPM | TUNE | waveform`.

- **BPM**: type 40–240 or `0` for automatic estimate; `?` indicates unknown BPM.
- **TUNE**: type integer -12..+12. Tune is sampled when a voice triggers; already playing voices keep their initial tuning.
- **SYNC**: use Live BPM for the sample's timeline while preserving the chosen pitch.
- **FREE**: do not tempo-match the sample, keeping its native sample timing.
- **FULL/5ms**: sustain the chosen slice or release the slot's voices at the next sequencer step.
- Waveform dragging sets start window. Shift+drag sets duration window. GENERATE uses those ranges for future slice events.

## Windows build

```powershell
cd C:\Users\picopicoDragon\Documents\Phraser
git fetch origin
git switch -c feature/v08-independent-pitch-stretch --track origin/feature/v08-independent-pitch-stretch
cmake -S . -B build-v08 -G "Visual Studio 18 2026" -A x64
cmake --build build-v08 --config Release --target Phraser_VST3
```

Output bundle directory: `build-v08/Phraser_artefacts/Release/VST3/Phraser.vst3`. Exit Live and back up the prior installed VST3 before updating `C:\Program Files\Common Files\VST3\Phraser.vst3`.

## Limitations

- WAV import remains synchronous, and sample ownership uses atomic shared_ptr; hard-realtime safety is not guaranteed.
- A total of 32 voices are supported, oldest voice stolen abruptly if exceeded; sums can clip.
- Granular OLA does not guarantee identical sound to source at extreme ratios; verify with ears before production use.
- Files are referenced by absolute paths, not embedded with the project.
- VST3 requires Ableton Live 12 / Windows; built on Visual Studio 2026 on PC1 or the GitHub Windows CI runner.

Detailed regression plan: [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md).
