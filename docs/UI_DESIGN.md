# v1.0 compact sample mixer and bar-sector orbit

Native JUCE UI: fixed 1280x850px, light gray/ivory studio palette and sharp 1-3px radii.

## Row layout

Each of the sixteen sample rows, left to right:

`SAMPLE NAME | MUTE | FULL/5ms | SYNC/FREE | BPM | TUNE | WAVEFORM | VOLUME+LEVEL`

The volume slider shares the same area as its peak-level meter, which is a thin green overlay. The WAV name can be clicked for file chooser, dragged onto a grid cell, or right-clicked to select a SOURCE ROOT (AUTO or C..B) for preference-key following. Explorer WAV drops still work on the name column.

## Upper right

Global controls remain in 3 rows of 4 rotary knobs. BARS is immediately beside STEPS. PREFERENCE KEY is in the header.

## Adaptive step grid

Left-edge track mute button **T1–T4** / **OFF** for each visible row. Sample-assigned cells use 16 unique source colors, not one generic green. Muted step cells show a faint source color and a small bright bottom marker, if the assigned source WAV is loaded.

BARS/STEPS:
- BARS1: 4 parallel tracks with 8 or 16 steps each.
- BARS2: 2 parallel tracks with 16 or 32 steps each.
- BARS3: 1 track with 24 or 48 steps.
- BARS4: 1 track with 32 or 64 steps.

## Overlap orbit

The orbit includes sixteen thin, colored concentric rings and switches between **1, 2, 3 or 4 equal bar sectors** according to BARS. Each bar sector has a B1..B4 label. Every enabled step becomes a colored angular arc on its sample's ring, with span approximating its individual duration. Active voices brighten their source ring, while muted sources dim. Thin radial boundaries indicate bar changes; a green ray indicates the currently active step and the center displays polyphony count.

The orbit does not generate a separate miniature circle for each bar; one larger circle is divided into bar sectors so all sixteen sources remain visible at once.
