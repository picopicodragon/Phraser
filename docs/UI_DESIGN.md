# v0.9 compact drag/key/parallel-track layout

The JUCE 1280×850 light-colored waveform workstation remains.

- HEADER: REFERENCE KEY dropdown (C ... B). The STEPS rotary knob is restored immediately beside BARS; it switches 8 or 16 steps per bar.
- SAMPLE NAME column: Click to open WAV picker; drag an existing name onto a phrase step; drop WAVs from Explorer directly onto the name row. A detected pitch-class suffix is displayed when available, and the automatic reference-key transposition is shown in the tooltip.
- Per-WAV FULL/5ms, SYNC/FREE, BPM and TUNE controls remain adjacent to each name.
- WAVEFORM: Drag for start selection and Shift+drag for length selection, retained.
- STEP GRID: Dynamic number of tracks/columns. BARS=1: 4×8 or 4×16. BARS=2: 2×16 or 2×32. BARS=3: 1×24 or 1×48. BARS=4: 1×32 or 1×64. All rows play in parallel, so the two- and four-track modes can layer samples at identical transport times.
- STEP INTERACTION: Drop a sample name onto a cell, or right-click to select from a menu of all 16 sample names. Left-click toggles enabled, Shift-click assigns currently selected sample.
- OVERLAP ORBIT: The circular view accounts for events in every enabled track.
- Dense 48/64 column layouts use narrow bars with accent dividers at each 8-step or 16-step bar; individual numeric labels are omitted at narrow widths.

All artwork and interaction code are Phraser-specific. No original phrz assets are included.
