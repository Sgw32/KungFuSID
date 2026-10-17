# Amiga sample candidate audition

Run from PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File firmware/amiga_samples/make_candidate_previews.ps1
```

This creates `audition_64` with WAV files numbered in the same order as
`AMIGA_SAMPLE_CANDIDATES.csv`.

- IFF/8SVX samples use their embedded sample rate and have their container
  stripped for the preview.
- Raw samples use a provisional 8,287 Hz PAL ProTracker C-3 playback rate.
- No trimming, normalization, resampling, or loop playback is applied.
- A sample that sounds incorrectly pitched may still be useful; its native
  rate and root note are among the metadata that must be set after audition.

When reducing the list to the first bank, record the chosen candidate IDs and,
for each one, desired trim, loop, root note, gain, and classification family.
