# v0.5 Waveform Matrix Design

The first Orbit layout was replaced by a sharper, denser production workstation interface, inspired by high-contrast sample engines but wholly original to Phraser.

## Appearance

- Fixed 1460×950 desktop canvas built with JUCE Graphics, no WebView.
- Graphite background (#0B1015) and low-chroma slate panels (#161D25).
- Single bright lime marker (#D5FA66) for playhead and critical emphasis.
- Waveforms use four restrained colour families across 16 slots rather than a neon rainbow.
- Tiny radii (2–5px), hairline dividers, compact uppercase section titles.

## Zones

- Header: PHRASER / WAVEFORM MATRIX v0.5.
- Left: 16 simultaneous waveform lanes; each lane has a numbered WAV load button and a per-sample peak envelope.
- Selected lane: pale amber overlay showing its random start window; start bounds shown as vertical dividers.
- Live playback: lime vertical marker per active sample, updated roughly 24 times/sec from audio-block atomic indicators.
- Right: 12 knobs, four per-sample randomness sliders, Generate and Keep/Recall A/B.
- Bottom: 4 rows × 16-step overview; inactive cells subdued when the selected total length is below 64.

## Interaction

- Click a WAV's button to select that slot and open file chooser.
- Click the waveform area to select its slot without reopening chooser.
- Edit per-sample normalized ranges and press GENERATE PHRASE; randomness is independent for each step and slot.
- Left-click bottom step to mute; Shift+click to assign selected WAV; right-click to LOCK.
- Steps-per-bar=8 makes each row represent two bars; steps-per-bar=16 makes each row one bar.
- All audio playback state is read from atomics and immutable snapshots, not via audio-buffer access from the UI.

## Pending design work

Window scaling, zoomable waveform inspector, colourblind-accessible patterns, MIDI keyboard navigation, keyboard focus and high DPI testing remain future work.
