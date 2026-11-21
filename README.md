# Phraser

Windows x64 VST3 sample-phrase generator for Ableton Live 12, JUCE 8.0.8 and C++17.

## v1.0.1 experimental: key follow, sample mixer and redesigned bar orbit

Development branch: `fix/v101-ui-contrast-meters-step-labels`, based on v1.0. Existing v1.0 work remains separate. This is a draft feature branch. Back up the installed working VST3 and use a test Live set until validation.

### PREFERENCE KEY / root tuning

- The C..B **PREFERENCE KEY** selector remains a host-stored parameter.
- WAVs with a confidently estimated dominant pitch class are automatically transposed to match that choice, with the shortest ±6-semitone shift.
- For WAVs without a reliable detected root, **right-click the SAMPLE NAME** to choose its SOURCE ROOT note C..B, or AUTO DETECT. The manual source-root override is persisted per sample in the Live project. This is the dependable way to force a key shift when detection fails.
- A sample's tooltip shows the effective root and actual semitone correction to the currently selected reference. Changed reference keys also update currently playing voices, without changing their tempo-related source-timeline speed.
- Per-source TUNE (-12..12), global PITCH and generated event pitch are still additive.
- An isolated sample's predominant pitch class is not necessarily a piece of music's harmonic key. No accurate major/minor key determination for polyphonic 2MIX is claimed.

### Distinct sample colors and the bar orbit

- All 16 WAVs have **different, stable colors** across the sample area, grid, and 16 concentric circular rings. Assigned steps retain a colored marker even when disabled.
- The lower-right orbit is a read-only musical timeline. The **circumference divides into 1, 2, 3 or 4 labeled BAR sectors**, following the BARS knob. Within every sector, colored arcs indicate enabled steps and their estimated sounding lengths.
- Ring number (inner to outer) represents SAMPLE 01..16; green ray indicates current step; the center displays active polyphonic voice count. Mutes dim source rings and hide their scheduled arcs. Orbit arcs remain a timing estimate, not a live oscilloscope.

### Track and sample mute

- A mute button sits at the left of each current sequencer track. The arrangement remains 4 tracks for BARS=1, 2 for BARS=2, and 1 for BARS=3/4.
- A separate mute button sits on each SAMPLE row. Both prevent new events from triggering and silence currently active voices while engaged.
- Track mute is stored for all four possible track indices and survives Live project save/load.
- Each of the 16 sample mutes is saved in the corresponding SampleSlot.

### Sample mixer: gain + embedded meter

- Every SAMPLE row contains an adjustable horizontal **VOLUME** slider (0..150%, initial 100%).
- A green peak-level indicator overlays the slider track. It reflects that sample's **actual mixed per-voice output contribution**, measured on the audio thread, with a short decay.
- Gain and meter are independent for each WAV. Sample gain is saved in the host project; meter values are live-only.
- Master OUTPUT remains available. Heavy overlapping voices can still clip; no limiter was added.

### Retained v0.9.1 functionality

- 16 WAVs, Explorer-to-SAMPLE file drops, SAMPLE-to-cell drops and right-click step assignment
- BARS 1–4, STEPS 8 or 16 per bar, adaptive track counts, up to 64 stored cells
- 32 independent overlapping voices, per-WAV FULL/5ms and SYNC/FREE
- BPM auto-estimation/manual override; pitch-independent granular resynthesis
- Waveform start/duration drag editing, Generate, Keep A/B, serialization

## Build (PC1)

```powershell
cd C:\Users\picopicoDragon\Documents\Phraser
git fetch origin
git switch -c fix/v101-ui-contrast-meters-step-labels --track origin/fix/v101-ui-contrast-meters-step-labels
cmake -S . -B build-v101 -G "Visual Studio 18 2026" -A x64
cmake --build build-v101 --config Release --target Phraser_VST3
```

VST3 directory: `build-v101/Phraser_artefacts/Release/VST3/Phraser.vst3`.

## v1.0.1 UI corrections

- Clear, dark PHRASE ENGINE numeric text and KEEP/RECALL button labels, independent of the host theme.
- All step cells show the assigned sample's 01..16 number, including disabled cells. The **near-white** background differentiates silent steps at a glance; a narrow color stripe retains sample identity.
- Reworked per-WAV VOLUME/LEVEL as one clean mixer strip with filled gain, overlayed live peak meter and percentage label, while keeping individual sample gain and audio-thread measurement unchanged.

## Limitations

- Automatic source-key estimation is conservative; manually enter SOURCE ROOT for uncertain/noisy/chordal WAVs. Musical-mode recognition (major/minor) is not implemented.
- The granular time-domain pitch stretcher may introduce transient smearing and flutter on complex sources.
- Changing reference root affects sounding voices immediately and could briefly produce a transition artifact; no pitch-glide smoothing is provided.
- Audio decoding, pitch/tempo detection and project-restoration file loads remain synchronous; this can block Live on long WAVs.
- A 32-voice cap remains, with immediate oldest-voice stealing at saturation. Layer summation can clip.
- Peak meters measure per-slot amplitude before final output clipping and are not LUFS or dBFS calibrated.
- Fixed-size 1280×850 panel; some controls are compact by design.

See [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) for QA gates.
