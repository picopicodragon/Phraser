# Phraser v0.6: Light Waveform Matrix + Overlap Orbit

The design retains v0.5's 16 independent waveform lanes and precise step editing while adding a circular musical overlap overview.

## Visual language

- Warm off-white background #EDF2EF with pale panels #FCFDFB.
- Dark forest ink #1E3029 for typography, muted green-grey metadata.
- Single fresh green highlight #238B60 for playheads and primary emphasis.
- Four muted source families (sea-green, amber, violet, rose), each used across four sample slots.
- Hairline dividers, square components and minimal rounded corners for a sharper instrument-style appearance. Original artwork and branding, no copies of phrz assets.

## Layout (fixed 1460×1150)

- Top: PHRASER, version tag, file/lane overview.
- Left, upper: 16 sample-load buttons and precalculated peak waveforms with live playback markers.
- Right, upper: 12 engine knobs, per-slot start and length random range controls, generator and Keep A/B.
- Left, lower: 64-step editable grid, plus short explanation of independent playback.
- Right, lower: **16-ring polyphonic overlap visualization**. Arc position gives phrase trigger time, arc length approximates that voice's sampled duration in song time. Brightened rings indicate audible sample slots, center displays current active voice count, and a thin ray follows the transport.

## Controls

- Waveform click selects the sample for per-slot random-range editing.
- Sample button loads WAV.
- Bottom grid click toggles step, Shift-click assigns selected WAV, right-click locks step.
- Circular view is currently read-only, designed to explain musical overlap rather than replace the waveform/sequence editing tools.

## Future work

Resizable UI, keyboard accessibility, multi-voice per-sample playhead markers, adjustable layer count and a zoomable ring/waveform editor.
