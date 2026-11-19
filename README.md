# Phraser

Windows-only VST3 phrase sampler for Ableton Live 12. Developed in C++17 using JUCE 8.0.8.

## Status

**Experimental v0.6.1** is on `feature/v061-per-slot-playback-mode`, based on the 16-WAV waveform-matrix v0.6 branch. The known working main branch is untouched. This v0.6 code must pass Windows CI and Ableton 12 testing before it is released.

## New in v0.6.1: per-WAV playback behavior

Each of the 16 WAV slots can independently choose one of two **trigger behavior modes** from the sample range inspector:

- **PLAYBACK: FULL SLICE LENGTH** (default): each trigger runs until that event's programmed slice end. Multiple triggers overlap independently, up to the existing 32-voice cap.
- **PLAYBACK: 5ms FADE AT NEXT STEP**: the voice plays normally until the sequencer advances to the **next step**, then releases smoothly over about **5 milliseconds**. This also happens when the next step is a rest. If a slice ends before the next step, it ends naturally. The next step itself can retrigger the same WAV.

Playback mode is per sample slot, captured at note trigger time. Changing it affects **new notes**, not currently sounding ones. Existing custom per-slot START FROM/TO and LENGTH MIN/MAX remain independent. All mode selections are saved with the Live project as a `fadeAtNextStep` property in each `SampleSlot` XML node. Old project files default to FULL SLICE LENGTH. The sample loader buttons show **F** (5ms next-step fade) or **L** (full length) beside the sample number. The orbit preview approximates the shorter release period.

## New in v0.6

### Real polyphonic sample-slicing

- **Up to 32 independent sample voices** play simultaneously, rather than fading all previous voices as soon as a new step begins.
- Every triggered event independently follows its randomized sample start, sample length and pitch playback rate.
- Each voice naturally terminates at its selected slice end (or the end of its source file), with the existing short attack/end fade.
- Rest steps **do not** stop existing notes. Overlapping samples add together.
- If more than 32 voices are needed, the **oldest voice is stolen**. Voice stealing can click in a saturated pattern; future versions should fade stolen notes smoothly.
- Transport stop releases all voices. MIDI input is accepted for VST3 instrument compatibility, but note-triggered polyphony is not yet implemented.

### Brighter design with both views

- Preserves the **16 simultaneous waveform lanes** and 4×16 step grid from v0.5.
- Replaces the dark charcoal theme with an **off-white, soft mint, ink-green** palette, light sample panels and stronger waveform contrast.
- Adds a **16-ring circular overlap map** to the bottom-right area. Each ring represents one WAV source, inner = 01, outer = 16.
- An arc shows each enabled step's approximate start time and sounding duration within the repeating phrase. Several arcs can appear at once on different rings, making overlaps visually obvious.
- Currently sounding samples brighten their rings; the center shows **live active voice count**, and a line indicates the current phrase step.
- Waveform lane playback markers still show the furthest-progressed current voice of that sample; they do not draw each voice independently.

### Retained from v0.5

- Up to 16 WAV source files and precomputed waveforms
- 1 to 4 bars, 8 or 16 steps/bar (8–64 total)
- Per-sample START FROM/TO and LENGTH MIN/MAX random ranges
- Density / Breaks / Snap / Variation, pitch/rate and pitch spread
- Keep A/B, step edit and lock, DAW state restoration

## Build on Windows (Visual Studio 2026)

```powershell
cd C:\Users\picopicoDragon\Documents\Phraser
git fetch origin
git switch -c feature/v061-per-slot-playback-mode --track origin/feature/v061-per-slot-playback-mode
cmake -S . -B build-v061 -G "Visual Studio 18 2026" -A x64
cmake --build build-v06 --config Release --target Phraser_VST3
```

Output: `build-v06/Phraser_artefacts/Release/VST3/Phraser.vst3` (directory).
Back up the known working VST3 bundle first; exit Live before installing, and rescan VST3 after updating.

## Limitations / quality notes

- Source files are external absolute WAV paths; sound is not embedded into the Live project.
- Samples are loaded synchronously and state restore may block the host; shared pointer ownership on the audio thread is not guaranteed hard realtime safe.
- Maximum 32 simultaneous voices, with oldest-voice stealing at the limit; no per-voice modulation or independent MIDI-note instruments yet.
- Mix of multiple voice outputs can clip. Reduce OUTPUT gain and avoid overly hot WAVs until a proper limiter/master stage is implemented.
- Slice duration is proportional to original WAV length, adjusted by playback rate; no separate time stretching or pitch correction.
- Ring arcs use predicted slice lengths at the currently reported BPM, so the visual is an approximation of audible overlap. 4/4 meter assumed.
- UI currently fixed 1460×1150, so small laptop displays may require OS scaling or later resize support.
- v0.6.1 must be checked in Ableton Live 12 before claiming production reliability.

See docs/DEVELOPMENT.md and docs/UI_DESIGN.md.
