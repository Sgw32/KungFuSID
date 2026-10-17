# KungFuSID Amiga backend and sample-bank draft

Status: the 16-instrument prototype backend, version-1 sample-bank format,
dual-slot firmware loader, factory image, and `kfsid_prog` bank flasher are
implemented. The 64-instrument expansion and NN classifier remain postponed.

## Desktop simulator and selected pilot bank

The 16 pilot instruments have now been selected and normalized without
changing the curated category folders. Run
`firmware/amiga_samples/prepare_selected_bank.ps1` to regenerate
`firmware/amiga_samples/selected_16`. Its `bank.csv` records the exact selected
path, ST-XX source, complete source SHA-256, decoded size/rate/loop data, and
provisional classifier metadata for each instrument.

The extensionless selections were identified by exact complete-file hashes:

| Selected name | Identified origin | Normalized bank name |
|---|---|---|
| `DeepBass` | `ST-01/DeepBass` | `03_ST-01_DeepBass.wav` |
| `Blast` | `ST-01/Blast` | `05_ST-01_Blast.wav` |
| `orchestralbass` | `ST-55/orchestralbass` | `06_ST-55_orchestralbass.wav` |
| `DigDug` | `ST-01/DigDug` | `09_ST-01_DigDug.wav` |
| `PolySynth` | `ST-01/PolySynth` | `10_ST-01_PolySynth.wav` |
| `organ1` | `ST-83/organ1` | `11_ST-83_organ1.wav` |

`firmware/amiga_samples/amiga_simulator` is the first host-side implementation
of the prototype backend. It uses libsidplayfp/reSIDfp to execute a SID tune,
samples its public SID register image, classifies gate-on events, and writes a
WAV through a shared three-channel sample engine. It implements provisional
SID ADSR mapping, pitch conversion, one-shot/loop playback, 64-step Paula
volume, coarse output quantization, oscillator sync, ring modulation, and a
simple SID-filter approximation. This is deliberately separate from the main
firmware so mappings and sample edits can be evaluated before IRQ-constrained
STM32 code is introduced.

The simulator classifier is a transparent hand-written baseline, not yet the
bank-centroid or NN backend described below. Its event trace is intended to
provide the first correction dataset. Public libsidplayfp 2.x exposes a
register snapshot rather than every write, so the default polling interval is
256 frames (about 5.8 ms at 44.1 kHz) and can miss intermediate
register states or shorter complete gate pulses. libsidplayfp 2.3 was verified
to corrupt memory when repeatedly called with smaller old-API output buffers,
so the simulator rejects them. A later cycle-accurate capture tool should use
a custom SID builder against libsidplayfp internals instead of register polling.

## Goals

- Keep both Amiga backends independent from the SID and AY3 backends.
- Load a replaceable sample bank from a C64 disk device using `kfsid_prog`.
- Reserve 64 instrument slots in bank format version 1, while using only 16
  active instruments for the first sound-quality and classification trial.
- Retain enough metadata for deterministic SID-to-instrument classification.
- Make an interrupted bank update recoverable without damaging firmware,
  parameters, or the previously working bank.
- Play sample data directly from flash; sample data must never be copied into
  the limited runtime RAM.

## Staged implementation decision

The 64-instrument bank is deliberately postponed, not removed. Version 1 of the
format reserves a fixed directory of 64 entries so a later 64-instrument bank
does not require a format or firmware redesign. The first implementation will
populate only 16 entries and mark the remaining 48 entries unused.

The 16-instrument pilot answers the most important unknown first: whether SID
music mapped onto Amiga-style samples is musically convincing. It also makes
manual tuning, loop correction, classifier debugging, and direct listening
comparisons manageable. If the result is successful, a later bank can activate
up to all 64 entries without changing the playback architecture.

The prototype backend is selectable now; the NN variant remains planned:

| Backend | Planned value | Classifier | Playback engine |
|---|---:|---|---|
| SID | 0 | Existing SID behavior | Existing SID engine |
| AY3 | 1 | Existing SID-to-AY mapping | Existing AY3 engine |
| Amiga Prototype | 2 | Deterministic SID patch classifier | Virtual Paula engine |
| Amiga NN | 3 | Tiny fixed-point neural network | Shared virtual Paula engine |

Selection remains a boot-time operation: changing the backend becomes effective
only after saving parameters and restarting KungFuSID. If no valid bank is
available, value 2 safely falls back to the SID backend.

Both Amiga backends must use exactly the same sample bank, feature extractor,
pitch conversion, envelopes, effects, mixer, and output scaling. Only the
classifier differs. This makes A/B listening meaningful and prevents a mixer
difference from being mistaken for a classifier improvement.

## Hardware and flash constraints

The target STM32F405RG has 1 MiB of internal flash. Its sectors are uneven:

| Sectors | Address range | Size | Proposed use |
|---|---:|---:|---|
| 0-3 | `0x08000000-0x0800FFFF` | 64 KiB | Existing firmware update area |
| 4 | `0x08010000-0x0801FFFF` | 64 KiB | Reserved for future recovery/factory data |
| 5-7 | `0x08020000-0x0807FFFF` | 384 KiB | Sample-bank slot A |
| 8-10 | `0x08080000-0x080DFFFF` | 384 KiB | Sample-bank slot B |
| 11 | `0x080E0000-0x080FFFFF` | 128 KiB | Existing parameter storage; do not share |

The firmware executes from SRAM after boot, which makes erasing and programming
the sample sectors practical. Audio must nevertheless be muted throughout an
erase/program operation, and no sample pointer may be dereferenced while its
slot is being modified.

### Why two 384 KiB slots

An A/B layout is preferred to one approximately 832 KiB bank. A new bank is
written into the inactive slot, read back, CRC-checked, and committed only after
it is complete. Loss of power leaves the previous slot valid. On boot, firmware
chooses the valid committed bank with the newest generation number.

The cost is a 384 KiB maximum bank instead of approximately 832 KiB. That is a
good trade for the first version: the 64 candidates below occupy 364,941 bytes
as source files, including IFF headers. A curated and trimmed 16-instrument bank
should normally occupy 80-200 KiB.

Sector 4 remains unallocated in version 1. It could later hold a very small
factory/recovery bank or an update journal, without changing the A/B format.

## Bank file

Use a dedicated extension such as `.KSB` (KungFuSID Sample Bank). It must not be
accepted by the firmware `.BIN` updater. The bank file is a complete image of
one slot, but it may be shorter than 384 KiB; erased trailing flash remains
`0xFF`.

Version 1 stores mono signed 8-bit PCM without compression. This matches Amiga
sample data, minimizes IRQ work, and permits direct flash playback. Compression
can be added in a later format version, but decoding compressed samples inside
the audio IRQ should be avoided.

### Proposed 64-byte bank header

All integers are little-endian in the generated `.KSB` file.

| Offset | Size | Field |
|---:|---:|---|
| 0 | 8 | Magic: `KFSAMP1\0` |
| 8 | 2 | Format version, initially 1 |
| 10 | 2 | Header size, initially 64 |
| 12 | 4 | Monotonically increasing generation |
| 16 | 4 | Total meaningful bank bytes |
| 20 | 2 | Active instrument count, initially 16 and at most 64 |
| 22 | 2 | Directory-entry size, initially 64 |
| 24 | 4 | Directory offset |
| 28 | 4 | PCM data offset, aligned to 256 bytes |
| 32 | 4 | Directory CRC32 |
| 36 | 4 | PCM-data CRC32 |
| 40 | 4 | Header/bank flags |
| 44 | 16 | Short bank name, UTF-8/ASCII and zero padded |
| 60 | 4 | Commit marker, programmed last |

The CRC calculation excludes the commit marker. An erased commit word is
`0xFFFFFFFF`. After all verification succeeds, a non-erased constant such as
`0x4B53424B` is programmed into that word without erasing the sector again.

### Proposed 64-byte runtime instrument entry

| Offset | Size | Field |
|---:|---:|---|
| 0 | 4 | PCM offset relative to bank base |
| 4 | 4 | PCM length in bytes/samples |
| 8 | 4 | Loop start in samples |
| 12 | 4 | Loop length; zero means one-shot |
| 16 | 4 | Native sample rate in Hz |
| 20 | 4 | Root frequency in unsigned Q16.16 Hz |
| 24 | 2 | Playback flags |
| 26 | 1 | Instrument family |
| 27 | 1 | Timbre/variant number |
| 28 | 1 | Default volume, 0-64 |
| 29 | 1 | Brightness, 0-255 |
| 30 | 1 | Preferred SID waveform mask |
| 31 | 1 | Drum subclass or zero |
| 32 | 16 | Runtime display/debug name |
| 48 | 16 | Signed 8-bit nearest-prototype feature centroid |

Suggested playback flags include `ONE_SHOT`, `LOOP`, `PERCUSSION`,
`ALLOW_PITCH_SWEEP`, `ALLOW_RING`, and `ALLOW_SYNC`.

Format version 1 always reserves 64 entries, or 4,096 bytes, even when only 16
are active. An unused entry has zero PCM length and an `UNUSED` flag. The PCM
area therefore begins at the same aligned offset for 16- and 64-instrument
banks. `instrument_count` reports active entries, not allocated slots.
Active entries occupy indices `0..instrument_count-1`; the remaining entries
must be unused. The directory CRC covers all 64 entries so corrupt unused slots
cannot later be mistaken for valid instruments.

Each centroid byte corresponds to one normalized feature from the shared
feature extractor. The value `-128` means that the feature is irrelevant for
that instrument; normal feature values use `-127..127`. Exact feature order is
part of the bank-format ABI and is described below.

The runtime directory deliberately excludes source paths, hashes, equipment
claims, and editing notes. Those belong in a sidecar manifest used by the bank
builder. This keeps flash lookup fixed-size and cheap.

## Host-side bank builder

A later bank-building tool should consume a CSV or JSON manifest and emit:

1. A validated `.KSB` image.
2. A human-readable build report.
3. WAV previews for auditioning.
4. The normalized metadata used to build the directory.

Required input processing:

- Read raw signed 8-bit samples and IFF/8SVX files.
- Strip IFF chunks and retain only decoded PCM.
- Import valid `VHDR` sample rate and loop points.
- Treat missing raw-file rate and loop information as unknown, not factual.
- Permit an explicit preview/native rate override.
- Trim silence and allow non-destructive start/end edits.
- Find or accept zero-crossing loop points.
- Apply gain offline; do not normalize dynamically in the IRQ.
- Align PCM starts to at least four bytes, preferably 16 or 32 bytes.
- Always emit the fixed 64-entry directory, filling unused entries explicitly.
- Reject banks over 393,216 bytes, invalid loops, duplicate IDs, and more than
  64 active instruments in format version 1.
- Generate CRC32 values. Distributed/factory `.KSB` files carry the commit
  marker; the device deliberately replaces it with an erased word while
  uploading and programs the marker only after read-back validation.

For raw ProTracker-style material, 8,287 Hz is a useful PAL C-3 audition
default, but it is not reliable source metadata. Root note and tuning must be
confirmed by listening. Some supplied IFF files explicitly use 8,363, 10,000,
or 16,726 Hz and should retain those rates unless deliberately resampled.

## Device-side bank selection

At boot, the bank loader should validate both slots without copying their PCM:

1. Check magic, version, bounds, entry size, count, and commit marker.
2. Validate directory CRC immediately.
3. Validate full PCM CRC once at boot, or cache a validated generation in RAM.
4. Reject entries whose data or loop range falls outside the bank.
5. Select the valid bank with the newer generation, handling wraparound.
6. Point the Amiga backend at its directory and flash PCM directly.

If neither slot is valid, selecting the Amiga backend should fail safely. The
preferred behavior is to fall back to the SID backend and expose a `No sample
bank` state through the programmer. Silence is less helpful for diagnosis.

Bank selection, CRC validation, instrument classification, pitch division, and
loop setup must occur outside the sample-mixing IRQ.

## C64 upload protocol

Sample-bank upload should be a separate protocol mode with a distinct start
magic from firmware update and parameter editing. Reusing the physical
KungFuSID register interface is fine; reusing firmware sector numbering is not.

Implemented version-1 sequence:

1. `kfsid_prog` opens a `.KSB` and validates its header and local CRCs.
2. KungFuSID mutes audio and selects the inactive slot.
3. KungFuSID erases that slot's three 128 KiB sectors.
4. The C64 streams 256-byte chunks directly from disk. The complete bank never
   needs to fit in C64 RAM or KungFuSID RAM.
5. Each chunk receives an XOR checksum acknowledgement.
6. At end-of-file, it validates all header fields, directory bounds, and the
   complete bank CRCs.
7. KungFuSID programs the commit marker last and acknowledges completion.
8. The new bank becomes effective after KungFuSID restart. Until then, the old
    slot remains the active source of any sample pointers.

Use a small SRAM staging buffer, for example 256 or 1,024 bytes. A 128 KiB
sector buffer is unnecessary and would not fit the current memory layout.
Because firmware runs from SRAM, flash programming can occur between protocol
handshakes. Interrupts should be disabled only for the actual short flash write,
not while waiting for disk data.

The programmer validates the file CRC before asking for confirmation, displays
the bank name and instrument count, and uses the main-screen progress bar while
streaming. Installed-bank metadata/capability queries are deferred to a future
protocol revision.

## Shared Amiga playback engine

The two Amiga backends are thin classifier front ends around one shared virtual
Paula playback engine. Its state contains three SID-driven sample channels and
optionally a fourth one-shot/effect channel. A channel holds only its current
entry pointer, fixed-point sample position/increment, loop state, quantized
volume, classification result, and modulation state.

The backend split should be made at note/instrument selection, not inside the
sample mixer:

```text
SID register writes
        |
        v
shared feature/event extractor
        |
        +---- Amiga Prototype classifier ----+
        |                                     |
        +---- Amiga NN classifier ----------- +
                                              |
                                              v
                                  selected bank entry
                                              |
                                              v
                                  shared virtual Paula mixer
                                              |
                                              v
                                          DAC output
```

At boot, backend value 2 installs the prototype note-selection handler and
backend value 3 installs the neural-network note-selection handler. The audio
cycle handler may be the same shared function for both. There must be no
prototype-versus-NN branch in the audio IRQ.

The audio handler should do only bounded integer work:

- Fetch up to four signed PCM bytes directly from flash.
- Advance fixed-point positions.
- Perform bounded loop/end checks.
- Apply SID ADSR mapped to Paula volume 0-64.
- Apply optional ring/sync operations.
- Sum, clip, optionally apply a coarse low-pass, and write the DAC value.

No sample search, feature extraction, classifier inference, file parsing, CRC,
division, allocation, or decompression may occur in the audio handler.

## Shared feature and event extractor

Both classifiers consume the same versioned 16-element signed 8-bit feature
vector. This ensures that they observe identical SID behavior and permits fair
comparison. Proposed feature order for format/classifier ABI version 1:

| Index | Feature |
|---:|---|
| 0 | Waveform character encoded from triangle/saw/pulse/noise combination |
| 1 | Logarithmic initial oscillator frequency |
| 2 | Frequency range during the observation window |
| 3 | Signed initial frequency delta |
| 4 | Downward pitch-sweep strength |
| 5 | Pulse width |
| 6 | Pulse-width movement |
| 7 | Attack rate |
| 8 | Decay rate |
| 9 | Sustain level |
| 10 | Release rate |
| 11 | Gate duration/retrigger character |
| 12 | Filter cutoff/brightness |
| 13 | Filter resonance and routing character |
| 14 | Noise/percussive likelihood |
| 15 | Ring/sync/test and other special behavior |

Categorical fields such as preferred waveform, family, and drum subclass remain
separate in each directory entry. The feature vector represents continuously
comparable characteristics.

Recognition has two phases:

1. At gate-on, create an immediate vector from currently known registers and
   start either a safe provisional sample or a cached prior result.
2. For approximately 1-3 ms, accumulate pitch movement, pulse movement, and
   retrigger evidence. Classify once, then lock the selected instrument until
   gate-off or a clearly defined retrigger.

A small signature cache, for example 16 entries, maps repeated SID patch/event
signatures to their last confident result. After the first occurrence, common
notes can select immediately without the observation delay.

Classification work belongs in a deferred control path or a bounded note-event
handler, not the audio cycle. The register-write path should capture features
and enqueue at most a small fixed-size event. It must not perform unbounded
searches while servicing the C64 bus.

Every classifier result contains:

- Selected active bank-entry index
- Confidence value
- Chosen family
- Whether the result is provisional or locked

Low-confidence results use a deterministic family default from bank metadata.
They must never select an unused or structurally invalid entry.

## Amiga Prototype backend

The prototype backend performs bank-driven weighted nearest-centroid matching.
It contains no trained model. For every active entry compatible with the hard
constraints, it computes a bounded distance:

```text
distance = sum(weight[i] * abs(event_feature[i] - centroid[i]))
```

Features whose centroid is `-128` are ignored. Preferred waveform, percussion
status, frequency range, one-shot/loop role, and family can reject an entry or
add a configurable penalty before centroid scoring.

Version 1 uses one fixed, versioned table of 16 integer feature weights in
firmware. The bank supplies centroids, not arbitrary weights or executable
logic. A later format may add family-specific weights if listening tests show
that they are necessary.

The entry with the smallest distance wins if its normalized score passes the
confidence threshold. Ties are resolved by stable entry ID, never randomly.
This means a replaceable bank carries its own classification profiles and can
be tuned without reflashing firmware.

For the 16-instrument pilot, checking all 16 entries is trivial. Scanning all 64
entries later is still inexpensive because it happens only at note events:
64 entries by 16 features is at most 1,024 small integer comparisons and
multiply/accumulates.

Prototype-backend advantages:

- Immediately inspectable and adjustable behavior
- No labelled training corpus required
- Classification metadata travels with the bank
- New banks do not require a new firmware model
- Produces baseline decisions and correction data for NN training

## Amiga NN backend

The NN backend uses a tiny offline-trained, fixed-point model. It consumes the
same 16-element event vector and predicts semantic families or traits; it does
not directly predict one of 64 physical sample IDs. Direct 64-way output would
couple the model to one bank and make a replacement bank incompatible.

A suitable first model is:

```text
16 signed-int8 inputs
        -> 16 signed-int8 hidden units with ReLU
        -> family logits and confidence
```

This is only a few hundred multiply/accumulate operations and roughly 0.5-1
KiB of quantized weights. It should be evaluated once per note event, never per
audio sample. Floating point, dynamic allocation, and a general ML runtime are
unnecessary; training happens on a desktop and exports fixed arrays plus scale
constants.

After the NN chooses a family, final variant selection uses the bank entries in
that family. A small nearest-centroid comparison over those variants is allowed
and keeps sample-bank replacement possible. The NN is therefore responsible
for musical interpretation; bank metadata remains responsible for choosing the
best available recording of that interpretation.

The model must declare a classifier ABI/model version. A bank may declare the
minimum compatible feature ABI, but it must not embed executable model code.
If the model or bank is incompatible, the NN backend falls back to the
prototype classifier and exposes the fallback status through `kfsid_prog`.

NN training data should be collected from prototype-backend trials:

1. Record feature vectors and selected entries from representative SID tunes.
2. Mark incorrect choices and supply the intended family.
3. Balance percussion, bass, lead, pad, key, and effect examples.
4. Split validation by tune/composer rather than randomly splitting notes from
   the same tune, which would give misleading accuracy.
5. Quantize and test the model offline against the exact firmware integer math.
6. Accept the model only if it improves unseen-tune accuracy over the prototype
   baseline without increasing missed percussion events.

There is no online learning in firmware. It would make behavior unpredictable,
complicate persistence, and provide no trustworthy labels.

## Comparing the two Amiga backends

Both backends must be tested with the same 16-entry bank and the same tunes.
Useful comparison metrics are:

- Correct family, especially kick/snare/hat separation
- Stable choice for repeated patches
- False drum detection rate
- Note-on classification latency
- Low-confidence/fallback count
- Audible clicks caused by provisional-to-final switching
- DWT cycles per classification and maximum C64 write-handler latency
- Subjective musical quality and preservation of the original arrangement

For diagnostics, a future debug build may expose counters or a compact event
trace. Diagnostic logging must be disabled in normal timing-critical builds.

## Classification metadata

Recommended instrument families:

- `KICK`, `SNARE`, `CLAP`, `HAT_CLOSED`, `HAT_OPEN`, `TOM`, `PERCUSSION`
- `BASS_SOFT`, `BASS_HARD`, `BASS_SLAP`
- `LEAD`, `BRASS`, `WOODWIND`
- `STRINGS`, `PAD`, `CHOIR`
- `PIANO`, `ORGAN`, `BELL`, `MALLET`, `PLUCK`
- `CHIP`, `EFFECT`

The sidecar manifest should eventually also contain root note, verified loop
points, brightness, attack character, sustain character, preferred SID
waveform, frequency range, drum-recognition features, gain, and provenance.

Equipment names in the supplied filenames are treated only as source hints.
For example, `d50`, `dx`, `juno`, `korg`, `moog`, `cz`, and `linn` strongly
suggest particular instruments or families, but they are not proof of origin.
Copyright and redistribution permission for the pilot 16 and any later expanded
bank must be checked separately.

## Curated 64-sample audition set and postponed expansion

The machine-readable inventory is in
`firmware/amiga_samples/AMIGA_SAMPLE_CANDIDATES.csv`. It records source path,
source size, decoded/container information where present, a short SHA-256,
provisional family, playback role, and filename-derived source hint.

Selection summary:

| IDs | Group | Count |
|---|---|---:|
| 01-20 | Kicks, snares, claps, hats, shaker and tom | 20 |
| 21-30 | Bass instruments | 10 |
| 31-40 | Leads, brass and winds | 10 |
| 41-50 | Strings, pads and choir | 10 |
| 51-60 | Bells, mallets, flute, piano, organ and plucks | 10 |
| 61-64 | Chip and effect candidates | 4 |

The 64 original files total 364,941 bytes. This is intentionally close to, but
below, one 384 KiB slot. IFF headers will be removed by the future bank builder,
leaving room for the 64-byte header, fixed 4,096-byte directory, and alignment.
The projected decoded image with conservative per-sample alignment is about
369 KiB, leaving about 24 KiB of slot headroom.

These 64 entries are candidates and reserved capacity, not the first production
bank. Implementation proceeds with 16 active entries. The other 48 directory
slots remain explicitly unused until the 16-sample experiment demonstrates
that the mapping sounds worthwhile and reveals which families need more
variation.

A balanced pilot bank should aim for approximately:

| Role | Suggested pilot count |
|---|---:|
| Kick, snare, closed hat, open hat | 4 |
| Soft, hard and slap/tonal bass | 3 |
| Pulse/square lead, saw lead and brass/wind | 3 |
| Strings/pad and choir | 2 |
| Organ or key | 1 |
| Bell and pluck/mallet | 2 |
| Noise/effect/orchestral hit | 1 |
| **Total** | **16** |

The exact IDs are selected after listening. Both Amiga backends must use this
same pilot bank. If the hypothesis is successful, expansion should be driven by
observed classifier ambiguity: add variants to families that are repetitive or
frequently fall back, rather than filling all 64 slots merely because they
exist.

Expansion beyond 16 is approved only after the pilot demonstrates:

- Recognizable and musically useful Amiga character on several unrelated tunes
- Acceptable kick/snare/hat and tonal-family selection
- Stable repeated-note classification without distracting sample changes
- No C64 bus or audio timing regressions at multiplier 10
- Useful differences between samples after transposition, rather than 64 names
  that collapse into nearly identical playback
- Clear evidence that additional variants improve variety or classification
  coverage

For listening to the supplied preview set:

- Use the numbered WAV files in `firmware/amiga_samples/audition_64`.
- Regenerate them with `firmware/amiga_samples/make_candidate_previews.ps1`.
- IFF/8SVX previews use their embedded rate; raw previews use provisional
  8,287 Hz PAL C-3 playback.
- Header rate and loop data marked `unknown` must be decided during the final
  16-sample edit.

## Pilot implementation order

After the 16 samples are chosen, implementation should proceed in this order:

1. Finalize trim, loop, tuning, gain, and 16 prototype centroids.
2. Build the fixed-64-slot `.KSB` generator and validate the bank offline.
3. Implement A/B bank validation and upload, without an Amiga audio backend.
4. Implement the shared virtual Paula mixer with explicit instrument selection
   test hooks.
5. Implement the shared feature/event extractor and signature cache.
6. Implement backend 2, Amiga Prototype, and tune it through listening tests.
7. Capture and label events from those tests.
8. Train and validate the tiny fixed-point model offline.
9. Implement backend 3, Amiga NN, using the identical pilot bank and mixer.
10. Compare both classifiers before deciding whether to expand toward 64 active
    instruments.

## Decisions needed after audition

For each of the final approximately 16 instruments, record:

1. Keep/reject decision and desired family.
2. Exact trim start and end.
3. One-shot or looped playback.
4. Loop start and length after trimming.
5. Native rate and musical root note.
6. Default gain and brightness.
7. Preferred SID waveform and ADSR/drum signature.
8. Permission/provenance status.

Only after these decisions should the bank builder, upload protocol, flasher UI,
shared Paula engine, and two Amiga classifier backends be implemented.
