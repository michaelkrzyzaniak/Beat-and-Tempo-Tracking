# PVT Beat Tracker design

PVT owns development of this repository. The initial fork preserves the MIT
license, attribution, and useful causal tempo/beat observer while creating a
small independently authored front end we can extend and test without changing
FFT code, host callbacks, or PVT's project format for every new algorithm.

## Implemented boundaries

`PVTOnset.h` / `src/PVTOnset.c` is our stateless C11 onset-strength module. It
accepts current and previous single-sided magnitude spectra and returns a finite,
nonnegative strength. It does not allocate, mutate inputs, compress spectra,
filter audio, pick peaks, guess tempo, or report beat confidence. The host owns
history and chooses FFT/window/hop, compression, and a noise floor.

Three methods are implemented: positive spectral flux, neighbor-maximum flux,
and normalized frequency-weighted positive flux. Neighbor flux uses a radius of
one bin in the previous frame; it is a deliberately simple pitch-motion
suppression method, not a full implementation of a named research algorithm.
The meaning of one bin depends on FFT resolution. High-frequency flux weights by
`bin/(bins-1)` and is resolution-normalized. DC is excluded. Non-finite and negative
bins contribute zero. Summation saturates at `FLT_MAX`. Array sizes remain the
caller's responsibility. See the public header for the complete contract.

`BTT` retains its opaque handle and default method. New
`btt_set_onset_detection_method` / `btt_get_onset_detection_method` APIs connect
our module to the existing observer. The default keeps its original float
accumulation order; changing methods clears spectrum history while preserving
the learned tempo. Configure before processing, or call `btt_clear()` before
starting a new stream. Calls on one tracker require external serialization;
independent instances do not share onset history.

Callback timestamps now advance at each actual STFT hop, regardless of host
buffer size. Negative corrected startup times clamp to zero. `btt_clear()` now
clears partial hops, spectral history, threshold/filter state, and prediction
indices. Metronome mode can run without a callback and reports its configured
tempo before any audio is analyzed. The inherited long-running beat suppression
counter no longer truncates to signed int.

## PVT integration

PVT applies the selected front end both to its independent local BTT observer
and to its higher-resolution Music feature/beat analysis, including named
frequency ranges. Its historical Hybrid choice still blends energy with spectral
changes. Other methods omit that blend. Configuration and cached provenance use
stable enum values, and changing a choice requires successful reanalysis.

The sibling PVT repository vendors explicit files using
`scripts/sync-beat-tracker.py`. Its CTest suite runs this repository's same C
regressions. The file list includes the MIT license and prevents silent drift
between the copy shipped in PVT and the maintained implementation.

## Validation and next extensions

The C suite uses spectra with known outcomes, invalid samples, saturation,
12-second pulse trains, callback identity across one-sample/irregular buffers,
reset/fresh-stream identity, and metronome null-callback operation. Native CMake
and a Linux/macOS/Windows CI matrix make those checks independent of PVT. Linux
CI additionally runs ASan/UBSan. Synthetic fixtures establish contracts and known
behaviors; they do not establish superiority on real music.

Next steps toward replacing inherited internals:

1. Build a licensed, annotated corpus with percussion, legato, vibrato, tempo
   ramps, syncopation, silence, and half/double-time ambiguity. Measure onset
   precision/recall, beat continuity, acquisition time, and tempo error by genre.
2. Implement a PVT-owned tempo-candidate scorer behind a separate API consuming
   onset history, exposing candidate confidence and explicit tempo bounds.
3. Implement beat-phase prediction against those candidates, then compare it
   against the retained observer on the corpus before changing defaults.
4. Share the new analyzer with Live capture using preallocated per-stream FFT
   state, bounded work, audio-thread-safe configuration swaps, and tested latency.

This change replaces the onset front end incrementally. The FFT, filtering,
adaptive threshold, tempo histogram, and beat predictor still contain inherited
code. Keep their attribution until those components are independently replaced.

## Completion review — 2026-09-10

The standalone native Release and ASan/UBSan tracker suites passed before the
interrupted handoff; those results were reused without repeating the tests.
Completion restored PVT's Windows `rand()` compatibility branch, explicitly
selected C11 in CMake, and enabled the retained helpers' Linux feature
declarations. The native C11 library build succeeds, and a compile-only check
of the Windows branch references `rand`. Native Windows/Linux CI remains pending.

The PVT vendor manifest matches all 16 maintained source/test files. The Music
selector is integrated, documented, and translated, and its dedicated Qt UI
check passes. PVT advances its pending public library ABI to 19 for the new
processing field. See the sibling PVT repository's `MUSIC_DETECTION.md` for
integration, migration, and detailed validation evidence. At completion, both repositories were uncommitted and the release was paused.
The PVT implementation ledger records subsequent release verification.
