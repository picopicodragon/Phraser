# v0.7 compact waveform editor

The v0.7 GUI preserves the v0.6 bright off-white / forest green style, while simplifying controls and reducing size from 1460×1150 to **1280×850**.

## Top-left: sixteen sample lanes

Each row includes:

1. Sample name / WAV load button.
2. **FULL / 5ms** button for playback end behavior.
3. **SYNC / FREE** button for detected-tempo resampling.
4. BPM field, editable (40–240, 0 resets to automatic).
5. Waveform with live sample playhead and two independently editable ranges.

No separate RANDOM WINDOWS inspector. Instead:
- Drag waveform horizontally to choose **start minimum and maximum**; translucent green overlay.
- Shift+drag to choose **duration minimum and maximum**; thin ochre line at the bottom of the waveform.
- Edit one WAV without changing ranges for other WAVs.
- Dragging any range affects **next GENERATE**; already-generated slices remain unchanged.

## Upper right

Twelve existing rotary controls for global phrase and sound settings, Generate and Keep A/B buttons.

## Bottom left

Four lines of sixteen step cells (inactive cells dimmed when phrase is shorter).

## Bottom right

Read-only circular overlap map; 16 rings show the predicted overlap at the currently detected Live tempo and per-source sync settings. Active sample rings brighten and current active-voice count appears at center.

## Design principles

Original Phraser branding, clear typography, sharp edges, pastel green highlights, restrained source colors and high density without hiding crucial editing controls. Interactive waveform range and per-row toggles take precedence over decorative elements.
