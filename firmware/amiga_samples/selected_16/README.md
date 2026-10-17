# Selected 16-sample pilot bank

This directory is generated from the user's curated selection by
`../prepare_selected_bank.ps1`. The curated category folders under
`../audition_64` remain the source of truth and are not renamed or modified.

All generated audio files are mono unsigned 8-bit PCM WAV. Their underlying
sample bytes remain the original signed 8-bit Amiga PCM. Raw samples use the
provisional PAL C-3 audition rate of 8,287 Hz; 8SVX samples retain their VHDR
rate and loop metadata. Tuning and automatically inferred loop points are used
by both the desktop simulator and the initial firmware bank and should still be
refined by listening tests.

Instrument 12, `ST-45/d50fantasia`, is intentionally tagged and rendered at
-24 semitones. Its WAV playback rate is 2,072 Hz instead of the 8,287 Hz source
rate, lowering it by two octaves without resampling or changing its PCM bytes.

`bank.csv` records each normalized file, its selected path, exact ST-XX source,
full source SHA-256, container, rate, classification role, and provisional
playback properties. The six extensionless files were matched to their ST-XX
origins by complete-file SHA-256 before conversion.

`../build_sample_bank.ps1` converts this directory into
`KungFuSID_Amiga_16.ksb`. Firmware builds append that bank at flash offset
`0x20000` in the `_all.bin` factory image. The ordinary firmware `.bin` remains
small and is still the correct file for in-system firmware updates.

