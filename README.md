# Phraser

Independent Windows x64 VST3 sample phrase instrument for Ableton Live 12. C++17 + JUCE 8.0.8.

## Status

- **main** is the last user-tested v0.2 prototype.
- `feature/phrase-engine-v03` is the v0.3 source (user reported no apparent problems).
- `feature/v04-orbit-ui-pitch` is **experimental v0.4**. **No Windows compile or Ableton runtime verification yet.** Do not distribute as a release.

## v0.4

- Entirely redrawn **Orbit UI** in native JUCE Graphics, not WebView2. Four concentric layer rings, each with sixteen segments. Orbit colour maps to one of four sample slots.
- Left-click within the orbit toggles the selected **step** (the step is shared across layers).
- **Shift+click** chooses the clicked sample layer for the step and enables it. **Right-click** toggles step lock. Yellow dots indicate locked steps.
- Center shows the Live transport's currently playing step.
- Right-hand dashboard groups ten knobs: Density, Slice Size, Breaks, Swing, Output, Fade, Transient Snap, Variation, Pitch and Pitch Spread.
- Four colour-coded WAV slot load buttons, NEW PHRASE, SAVE A/B and RECALL A/B.
- Global Pitch ±12 semitones plus per-step randomized pitch deviations from Pitch Spread 0–12 semitones. Resulting total shift limited to ±24 semitones.
- Pitch is implemented by changing sample playback increment. **Pitch and speed/duration change together**. This is **not** independent-pitch time stretching, nor key detection/correction.
- Pattern Keep A/B memories are saved inside the host state alongside the current phrase and sample paths.
- Retains previous features: four independent WAVs, onset approximation and snapping, eight-voice 5ms release overlaps, 16-step motif variation, sample reference persistence, MIDI input event bus.

The styling takes inspiration from the general interaction pattern of a multilayer circular sampler. Artwork, layout, branding and implementation are original and not copied from nok's product.

## Build on Windows with Visual Studio 2026

```powershell
git clone https://github.com/picopicodragon/Phraser.git
cd Phraser
git fetch origin
git switch -c feature/v04-orbit-ui-pitch --track origin/feature/v04-orbit-ui-pitch
cmake -S . -B build-v04 -G "Visual Studio 18 2026" -A x64
cmake --build build-v04 --config Release --target Phraser_VST3
```

VST3 bundle output: `build-v04/Phraser_artefacts/Release/VST3/Phraser.vst3`.

**Backup v0.3 first**, quit Live, and copy the complete VST3 bundle folder to `C:\Program Files\Common Files\VST3\Phraser.vst3` with administrator rights. Rescan in Live 12. Use a new test project.

## Known limitations

- Experimental. v0.4's source has not been compiled on a Windows toolchain as part of this change.
- No dedicated key detection, tempo-independent pitch shifting, time stretching, DAW clip export, MIDI note triggering or multibus stems.
- Sample files are saved by absolute path and remain external; lost or moved source files are silent.
- Disk file loads and state restore are synchronous and can temporarily block. Sample ownership via atomic shared_ptr can still incur realtime-thread destruction. These need a dedicated loader and deferred reclamation.
- Eight voices, but existing voices are released at every new step. The result is a short crossfade, not long independent polyphony.
- One-bar (16 sixteenth notes) sequencer, assuming 4/4.
- Floating-point output can exceed 1.0 when overlapping loud samples; user should manage output level until a limiter is added.

See [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) and [docs/UI_DESIGN.md](docs/UI_DESIGN.md).
