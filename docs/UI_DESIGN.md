# Phraser v0.8: per-source TUNE in compact waveform matrix

The 1280×850 light off-white interface remains. Every sample lane now reads:

`WAV name | FULL/5ms | SYNC/FREE | BPM | TUNE | waveform`

TUNE is an editable text field accepting integer semitones -12..+12, directly beside the BPM field. A positive value is displayed with a `+` prefix. Original pitch is `0`.

The waveform display starts at x=427 rather than x=363, with width reduced from 495 to 431 so all 16 TUNE fields fit without increasing the window. Waveform range editing, the step grid and 16-ring overlap map remain intact.

SYNC changes the playback timeline without transposing; TUNE changes the pitch independently. BPM detection and tuning are stored per source. A tuning change applies to newly triggered voices.
