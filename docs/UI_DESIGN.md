# Orbit UI direction (v0.4)

The interface is a native JUCE instrument panel at 1100×730. It intentionally adopts a unique dark slate / sea-glass visual language rather than copying phrz brand elements.

## Layout

- **Header:** Phraser, Orbit Engine v0.4, primary NEW PHRASE action.
- **Left stage:** four concentric circular rings, four distinct sample colours, sixteen divisions, playback highlight, locked-step markers, central playhead.
- **Bottom-left:** SAVE A / RECALL A / SAVE B / RECALL B buttons.
- **Right rack:** ten rotary controls in two rows, translucent dark panel, below which four coloured slot controls load WAVs.
- **Live status:** narrow message line at bottom right.

## Input behaviour

- Ring click toggles the step enabled state.
- Shift + ring click assigns that ring's sample slot to the step.
- Right-click ring segment toggles step LOCK.
- FREE / LOCK are not printed per segment; yellow dots communicate locked steps.
- NEW PHRASE regenerates only unlocked steps.
- SAVE A/B stores a full 16-step snapshot; RECALL A/B restores it.
- UI refreshes the current transport step 24 times/sec only when it changes.
- WAV load happens through the operating-system file chooser.

## Design constraints

- No web browser dependency or streamed visual content.
- No copied trademarks, images, logos, exact layout or font assets of phrz.
- High contrast, large visible targets for loading, generating and recalling.
- Default GUI remains a fixed-size experimental panel. User resizing, keyboard accessibility and high-DPI QA are planned improvements.
