# v0.7.1 hotfix: Live 12 crash during WAV import

## Root cause

In v0.7, Source/TempoDetection.h created `std::array<double, 161> scores`, but the BPM detector indexed it directly by the integer BPM from 70 through **180** inclusive. Indices **161 through 180** exceeded the array boundary, writing to invalid stack memory during WAV load. This can crash the entire Ableton Live host rather than just Phraser.

Fix: Allocate `maxBpm + 1` elements with explicit `minBpm=70` and `maxBpm=180`, and protect against nonfinite sample-rate and onset intervals. This hotfix contains no intended behavioral change to the audio engine or waveform UI.

## PC1 validation

```powershell
cd C:\Users\picopicoDragon\Documents\Phraser
git fetch origin
git switch -c fix/v071-wav-loading-crash --track origin/fix/v071-wav-loading-crash
cmake -S . -B build-v071 -G "Visual Studio 18 2026" -A x64
cmake --build build-v071 --config Release --target Phraser_VST3
```

### Acceptance tests

- [ ] GitHub Windows VST3 build succeeds.
- [ ] On a **new throwaway Live project**, load a short drum loop, then a longer percussion loop and a sustained WAV.
- [ ] Repeat WAV imports across slots 01–16 to verify host remains alive.
- [ ] Edit manual BPM and SYNC/FREE after import without crashing.
- [ ] Save/reopen the test project and verify WAV paths and tempos restore.
- [ ] Test switching WAV while transport is running, then after stopping.
- [ ] Confirm no regressions in 32-voice playback, sample fades and waveform drag.
- [ ] If Live still crashes, attach Live's Diagnostic Report and identify WAV format, duration, sample rate and slot. Crashes during UI reload or project restore may have separate causes.

## Remaining risks

Loading and decoding WAVs are still performed synchronously on the UI or host state-restore thread. This could stall Live and cause memory pressure when many long files are loaded. The 16-slot sample bank shares ownership with the audio callback. A future release should implement worker-thread import and realtime-safe sample handoff.
