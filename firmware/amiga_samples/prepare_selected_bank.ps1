param(
    [string]$OutputPath = (Join-Path $PSScriptRoot 'selected_16')
)

$ErrorActionPreference = 'Stop'

function Read-BigEndianUInt32([byte[]]$Data, [int]$Offset)
{
    return ([uint32]$Data[$Offset] * 16777216) +
           ([uint32]$Data[$Offset + 1] * 65536) +
           ([uint32]$Data[$Offset + 2] * 256) +
           [uint32]$Data[$Offset + 3]
}

function Read-BigEndianUInt16([byte[]]$Data, [int]$Offset)
{
    return ([uint16]$Data[$Offset] * 256) + [uint16]$Data[$Offset + 1]
}

function Read-AmigaSample([string]$Path, [int]$RawRate)
{
    $data = [IO.File]::ReadAllBytes($Path)
    $result = [ordered]@{
        Pcm = $data
        Rate = $RawRate
        Container = 'RAW'
        LoopStart = ''
        LoopLength = ''
    }

    if ($data.Length -lt 12 -or
        [Text.Encoding]::ASCII.GetString($data, 0, 4) -ne 'FORM' -or
        [Text.Encoding]::ASCII.GetString($data, 8, 4) -ne '8SVX')
    {
        return $result
    }

    $result.Container = '8SVX'
    $offset = 12
    while ($offset + 8 -le $data.Length)
    {
        $tag = [Text.Encoding]::ASCII.GetString($data, $offset, 4)
        $length = Read-BigEndianUInt32 $data ($offset + 4)
        $chunk = $offset + 8
        if ($chunk + $length -gt $data.Length) {
            throw "Invalid $tag chunk in $Path"
        }

        if ($tag -eq 'VHDR' -and $length -ge 20)
        {
            $oneShot = Read-BigEndianUInt32 $data $chunk
            $repeat = Read-BigEndianUInt32 $data ($chunk + 4)
            $result.Rate = Read-BigEndianUInt16 $data ($chunk + 12)
            if ($repeat -gt 0) {
                $result.LoopStart = $oneShot
                $result.LoopLength = $repeat
            }
        }
        elseif ($tag -eq 'BODY')
        {
            $pcm = New-Object byte[] $length
            [Array]::Copy($data, $chunk, $pcm, 0, $length)
            $result.Pcm = $pcm
        }

        $offset = $chunk + $length + ($length % 2)
    }
    return $result
}

function Write-Pcm8Wave([string]$Path, [byte[]]$SignedPcm, [int]$SampleRate)
{
    $wavePcm = New-Object byte[] $SignedPcm.Length
    for ($i = 0; $i -lt $SignedPcm.Length; ++$i) {
        $wavePcm[$i] = $SignedPcm[$i] -bxor 0x80
    }

    $stream = [IO.File]::Open($Path, [IO.FileMode]::Create,
        [IO.FileAccess]::Write, [IO.FileShare]::None)
    try {
        $writer = New-Object IO.BinaryWriter($stream)
        try {
            $writer.Write([Text.Encoding]::ASCII.GetBytes('RIFF'))
            $writer.Write([int](36 + $wavePcm.Length))
            $writer.Write([Text.Encoding]::ASCII.GetBytes('WAVE'))
            $writer.Write([Text.Encoding]::ASCII.GetBytes('fmt '))
            $writer.Write([int]16)
            $writer.Write([int16]1)
            $writer.Write([int16]1)
            $writer.Write([int]$SampleRate)
            $writer.Write([int]$SampleRate)
            $writer.Write([int16]1)
            $writer.Write([int16]8)
            $writer.Write([Text.Encoding]::ASCII.GetBytes('data'))
            $writer.Write([int]$SignedPcm.Length)
            $writer.Write($wavePcm)
        }
        finally { $writer.Dispose() }
    }
    finally { $stream.Dispose() }
}

# `SelectedPath` records exactly what was selected. `SourcePath` is the
# byte-identical ST-XX origin used to regenerate a uniform WAV bank.
$selection = @(
    @{ Id='01'; Pack='ST-01'; Name='KorgBass';        SelectedPath='audition_64/bass/22_ST-01_KorgBass.wav';          SourcePath='ST-01/KorgBass';        Family='BASS_HARD';  Playback='loop_auto'; Wave='pulse'; Root=48; Gain='0.90'; LoopStart=2430; LoopLength=1570; Notes='Korg-labelled hard bass' },
    @{ Id='02'; Pack='ST-60'; Name='ffjunobass';      SelectedPath='audition_64/bass/24_ST-60_ffjunobass.wav';        SourcePath='ST-60/ffjunobass';      Family='BASS_HARD';  Playback='loop_auto'; Wave='saw';   Root=48; Gain='0.90'; LoopStart=428;  LoopLength=1222; Notes='Juno-labelled bass' },
    @{ Id='03'; Pack='ST-01'; Name='DeepBass';        SelectedPath='audition_64/bass/DeepBass';                       SourcePath='ST-01/DeepBass';        Family='BASS_SOFT';  Playback='loop_auto'; Wave='tri';   Root=48; Gain='0.90'; LoopStart=4377; LoopLength=3702; Notes='Extensionless selection; exact origin identified by SHA-256' },
    @{ Id='04'; Pack='ST-01'; Name='Marimba';         SelectedPath='audition_64/bellstyle/53_ST-01_Marimba.wav';     SourcePath='ST-01/Marimba';         Family='MALLET';     Playback='one_shot';  Wave='tri';   Root=60; Gain='0.95'; Notes='Marimba transient' },
    @{ Id='05'; Pack='ST-01'; Name='Blast';           SelectedPath='audition_64/effect/Blast';                       SourcePath='ST-01/Blast';           Family='EFFECT';     Playback='one_shot';  Wave='noise'; Root=48; Gain='0.85'; Notes='Extensionless selection; exact origin identified by SHA-256' },
    @{ Id='06'; Pack='ST-55'; Name='orchestralbass';  SelectedPath='audition_64/effect/orchestralbass';             SourcePath='ST-55/orchestralbass';  Family='EFFECT';     Playback='one_shot';  Wave='mixed'; Root=48; Gain='0.85'; Notes='Extensionless selection; exact origin identified by SHA-256' },
    @{ Id='07'; Pack='ST-94'; Name='lead.korg31';     SelectedPath='audition_64/leads/35_ST-94_lead.korg31.wav';     SourcePath='ST-94/lead.korg31';     Family='LEAD';       Playback='loop';      Wave='saw';   Root=48; Gain='0.82'; Notes='8SVX loop metadata retained' },
    @{ Id='08'; Pack='ST-A6'; Name='korg15.short';    SelectedPath='audition_64/leads/40_ST-A6_korg15.short.wav';    SourcePath='ST-A6/korg15.short';    Family='LEAD';       Playback='loop_auto'; Wave='pulse'; Root=48; Gain='0.88'; LoopStart=3377; LoopLength=2561; Notes='Short Korg-labelled lead' },
    @{ Id='09'; Pack='ST-01'; Name='DigDug';          SelectedPath='audition_64/leads/DigDug';                       SourcePath='ST-01/DigDug';          Family='CHIP';       Playback='loop_auto'; Wave='pulse'; Root=60; Gain='0.82'; LoopStart=1911; LoopLength=1189; Notes='Extensionless selection; exact origin identified by SHA-256' },
    @{ Id='10'; Pack='ST-01'; Name='PolySynth';       SelectedPath='audition_64/leads/PolySynth';                    SourcePath='ST-01/PolySynth';       Family='LEAD';       Playback='loop_auto'; Wave='saw';   Root=48; Gain='0.82'; LoopStart=4621; LoopLength=5279; Notes='Extensionless selection; exact origin identified by SHA-256' },
    @{ Id='11'; Pack='ST-83'; Name='organ1';          SelectedPath='audition_64/organ/organ1';                       SourcePath='ST-83/organ1';          Family='ORGAN';      Playback='loop_auto'; Wave='pulse'; Root=48; Gain='0.78'; LoopStart=9603; LoopLength=12641; Notes='Extensionless selection; exact origin identified by SHA-256' },
    @{ Id='12'; Pack='ST-45'; Name='d50fantasia';     SelectedPath='audition_64/pads/41_ST-45_d50fantasia.wav';      SourcePath='ST-45/d50fantasia';     Family='PAD';        Playback='loop_auto'; Wave='saw';   Root=48; Gain='0.78'; Transpose=-24; LoopStart=5245; LoopLength=2743; Notes='D-50-labelled pad; shifted two octaves down' },
    @{ Id='13'; Pack='ST-13'; Name='CZSTRING';        SelectedPath='audition_64/pads/47_ST-13_CZSTRING.wav';         SourcePath='ST-13/CZSTRING';        Family='STRINGS';    Playback='loop_auto'; Wave='saw';   Root=48; Gain='0.78'; LoopStart=3936; LoopLength=5790; Notes='CZ-labelled string pad' },
    @{ Id='14'; Pack='ST-02'; Name='linnkick';        SelectedPath='audition_64/percussion/01_ST-02_linnkick.wav';  SourcePath='ST-02/linnkick';        Family='KICK';       Playback='one_shot';  Wave='tri';   Root=48; Gain='1.00'; Notes='Linn-labelled kick' },
    @{ Id='15'; Pack='ST-A5'; Name='d50-Snaredrum';   SelectedPath='audition_64/percussion/07_ST-A5_d50-Snaredrum.wav'; SourcePath='ST-A5/d50-Snaredrum'; Family='SNARE'; Playback='one_shot'; Wave='noise'; Root=48; Gain='0.95'; Notes='D-50-labelled snare' },
    @{ Id='16'; Pack='ST-A5'; Name='d50-Hihat';       SelectedPath='audition_64/percussion/13_ST-A5_d50-Hihat.wav'; SourcePath='ST-A5/d50-Hihat';       Family='HAT_CLOSED'; Playback='one_shot';  Wave='noise'; Root=48; Gain='0.90'; Notes='D-50-labelled closed hi-hat' }
)

New-Item -ItemType Directory -Path $OutputPath -Force | Out-Null
$manifest = @()
foreach ($entry in $selection)
{
    $selected = Join-Path $PSScriptRoot $entry.SelectedPath
    $source = Join-Path $PSScriptRoot $entry.SourcePath
    if (-not (Test-Path -LiteralPath $selected)) { throw "Missing selection: $selected" }
    if (-not (Test-Path -LiteralPath $source)) { throw "Missing source: $source" }

    # For extensionless selections, prove that the inferred ST-XX origin is exact.
    if ([IO.Path]::GetExtension($selected) -eq '') {
        $selectedHash = (Get-FileHash -LiteralPath $selected -Algorithm SHA256).Hash
        $sourceHash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
        if ($selectedHash -ne $sourceHash) {
            throw "Origin mismatch for $($entry.SelectedPath)"
        }
    }

    $sample = Read-AmigaSample $source 8287
    $transpose = if ($null -ne $entry.Transpose) { [int]$entry.Transpose } else { 0 }
    $outputRate = [int][Math]::Round($sample.Rate * [Math]::Pow(2.0, $transpose / 12.0))
    $filename = '{0}_{1}_{2}.wav' -f $entry.Id, $entry.Pack, $entry.Name
    $outputFile = Join-Path $OutputPath $filename
    Write-Pcm8Wave $outputFile $sample.Pcm $outputRate
    if ([IO.Path]::GetExtension($selected) -eq '.wav' -and $transpose -eq 0) {
        $selectedWaveHash = (Get-FileHash -LiteralPath $selected -Algorithm SHA256).Hash
        $outputWaveHash = (Get-FileHash -LiteralPath $outputFile -Algorithm SHA256).Hash
        if ($selectedWaveHash -ne $outputWaveHash) {
            throw "Selected WAV differs from regenerated source: $($entry.SelectedPath)"
        }
    }
    $sourceHash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant()

    $manifest += [pscustomobject][ordered]@{
        id = $entry.Id
        file = $filename
        family = $entry.Family
        playback = $entry.Playback
        source_pack = $entry.Pack
        source_path = $entry.SourcePath
        selected_path = $entry.SelectedPath
        source_sha256 = $sourceHash
        source_container = $sample.Container
        pcm_bytes = $sample.Pcm.Length
        source_rate_hz = $sample.Rate
        native_rate_hz = $outputRate
        transpose_semitones = $transpose
        root_midi = $entry.Root
        loop_start = if ($null -ne $entry.LoopStart) { $entry.LoopStart } else { $sample.LoopStart }
        loop_length = if ($null -ne $entry.LoopLength) { $entry.LoopLength } else { $sample.LoopLength }
        gain = $entry.Gain
        preferred_waveform = $entry.Wave
        notes = $entry.Notes
    }
}

$manifest | Export-Csv -LiteralPath (Join-Path $OutputPath 'bank.csv') -NoTypeInformation -Encoding UTF8
Write-Host "Prepared $($manifest.Count) normalized samples in $OutputPath"

