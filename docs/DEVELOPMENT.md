# v0.9.1 STEPS control regression tests

STEPS has been restored as the rotary knob immediately to the right of BARS, with two options: 8 / 16. It also controls host transport timing and grid columns. Previous v0.9 features are preserved.

## Build

```powershell
cd C:\Users\picopicoDragon\Documents\Phraser
git fetch origin
git switch -c feature/v091-restore-steps-control --track origin/feature/v091-restore-steps-control
cmake -S . -B build-v091 -G "Visual Studio 18 2026" -A x64
cmake --build build-v091 --config Release --target Phraser_VST3
```

## Test matrix

| BARS | STEPS | Tracks | Columns per track | Cells |
| --- | --- | --- | --- | --- |
| 1 | 8 | 4 | 8 | 32 |
| 1 | 16 | 4 | 16 | 64 |
| 2 | 8 | 2 | 16 | 32 |
| 2 | 16 | 2 | 32 | 64 |
| 3 | 8 | 1 | 24 | 24 |
| 3 | 16 | 1 | 48 | 48 |
| 4 | 8 | 1 | 32 | 32 |
| 4 | 16 | 1 | 64 | 64 |

- [ ] Windows Release VST3 CI succeeds.
- [ ] STEPS is visible adjacent to BARS and switches between 8 and 16 (only).
- [ ] STEP=8 triggers 8 events per 4/4 bar; STEP=16 triggers 16.
- [ ] Changing BARS or STEPS refreshes grid while stopped, including hit testing.
- [ ] Multiple tracks trigger at identical transport positions.
- [ ] Sample drag/drop and right-click assignment work at every setting.
- [ ] Generate, lock, Keep A/B and host state save/reopen work for both STEPS settings.
- [ ] Reference-key tuning and existing WAV-import crash fix still work.
- [ ] Live 12 tempo changes and transport seek do not cause stuck notes or crashes.
