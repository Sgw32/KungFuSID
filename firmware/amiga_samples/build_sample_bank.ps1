param(
    [string]$InputPath = (Join-Path $PSScriptRoot 'selected_16'),
    [string]$OutputPath = (Join-Path $PSScriptRoot 'selected_16/KungFuSID_Amiga_16.ksb'),
    [uint32]$Generation = 1,
    [string]$BankName = 'Amiga Pilot 16'
)

$ErrorActionPreference = 'Stop'
$HeaderSize = 64
$EntrySize = 64
$DirectoryEntries = 64
$DirectoryOffset = $HeaderSize
$PcmOffset = 4352 # align 64 + 64*64 to 256 bytes
$CommitMarker = [uint32]0x4B53424B

function Read-Le16([byte[]]$Data, [int]$Offset) {
    return [uint16]([int]$Data[$Offset] + ([int]$Data[$Offset + 1] -shl 8))
}

function Read-Le32([byte[]]$Data, [int]$Offset) {
    return [uint32]([int64]$Data[$Offset] + ([int64]$Data[$Offset + 1] -shl 8) +
        ([int64]$Data[$Offset + 2] -shl 16) + ([int64]$Data[$Offset + 3] -shl 24))
}

function Read-WavePcm8([string]$Path) {
    $data = [IO.File]::ReadAllBytes($Path)
    if ($data.Length -lt 44 -or [Text.Encoding]::ASCII.GetString($data, 0, 4) -ne 'RIFF') {
        throw "Not a WAV file: $Path"
    }
    $rate = 0
    $pcm = $null
    $offset = 12
    while ($offset + 8 -le $data.Length) {
        $tag = [Text.Encoding]::ASCII.GetString($data, $offset, 4)
        $length = Read-Le32 $data ($offset + 4)
        $body = $offset + 8
        if ($body + $length -gt $data.Length) { throw "Truncated WAV: $Path" }
        if ($tag -eq 'fmt ') {
            if ((Read-Le16 $data $body) -ne 1 -or (Read-Le16 $data ($body + 2)) -ne 1 -or
                (Read-Le16 $data ($body + 14)) -ne 8) {
                throw "Bank input must be mono PCM8: $Path"
            }
            $rate = Read-Le32 $data ($body + 4)
        } elseif ($tag -eq 'data') {
            $pcm = New-Object byte[] $length
            for ($i = 0; $i -lt $length; ++$i) { $pcm[$i] = $data[$body + $i] -bxor 0x80 }
        }
        $offset = $body + $length + ($length -band 1)
    }
    if ($rate -eq 0 -or $null -eq $pcm) { throw "Incomplete WAV: $Path" }
    return @{ Rate=[uint32]$rate; Pcm=$pcm }
}

function Get-Crc32([byte[]]$Data) {
    [uint64]$crc = 4294967295
    foreach ($byte in $Data) {
        $crc = ($crc -bxor [uint64]$byte) -band 4294967295
        for ($bit = 0; $bit -lt 8; ++$bit) {
            if ($crc -band 1) { $crc = (($crc -shr 1) -bxor 3988292384) -band 4294967295 }
            else { $crc = ($crc -shr 1) -band 4294967295 }
        }
    }
    return [uint32](($crc -bxor 4294967295) -band 4294967295)
}

function Write-FixedAscii([IO.BinaryWriter]$Writer, [string]$Text, [int]$Length) {
    $bytes = [Text.Encoding]::ASCII.GetBytes($Text)
    $count = [Math]::Min($bytes.Length, $Length)
    $Writer.Write($bytes, 0, $count)
    for ($i = $count; $i -lt $Length; ++$i) { $Writer.Write([byte]0) }
}

$familyIds = @{ BASS_HARD=1; BASS_SOFT=1; LEAD=2; PAD=3; ORGAN=4; MALLET=5;
    EFFECT=6; CHIP=7; KICK=8; SNARE=9; HAT_CLOSED=10; STRINGS=11 }
$waveMasks = @{ tri=0x10; saw=0x20; pulse=0x40; noise=0x80; mixed=0x70 }
$manifest = @(Import-Csv -LiteralPath (Join-Path $InputPath 'bank.csv'))
if ($manifest.Count -gt 64) { throw 'A version-1 bank supports at most 64 entries.' }

$directoryStream = New-Object IO.MemoryStream
$directoryWriter = New-Object IO.BinaryWriter($directoryStream)
$pcmStream = New-Object IO.MemoryStream
$pcmWriter = New-Object IO.BinaryWriter($pcmStream)
try {
    foreach ($entry in $manifest) {
        while (($pcmStream.Length -band 15) -ne 0) { $pcmWriter.Write([byte]0) }
        $wave = Read-WavePcm8 (Join-Path $InputPath $entry.file)
        $dataOffset = $PcmOffset + $pcmStream.Length
        $pcmWriter.Write([byte[]]$wave.Pcm)
        $isLoop = $entry.playback -ne 'one_shot'
        $isPercussion = $entry.family -in @('KICK','SNARE','HAT_CLOSED')
        [uint16]$flags = if ($isLoop) { 2 } else { 1 }
        if ($isPercussion) { $flags = $flags -bor 4 }
        $flags = $flags -bor 8 -bor 16
        $rootHz = 440.0 * [Math]::Pow(2.0, ([int]$entry.root_midi - 69) / 12.0)
        [uint32]$loopStart = 0
        [uint32]$loopLength = 0
        if ($entry.loop_start) { $loopStart = $entry.loop_start }
        if ($entry.loop_length) { $loopLength = $entry.loop_length }
        [byte]$drum = 0
        if ($entry.family -eq 'KICK') { $drum = 1 }
        elseif ($entry.family -eq 'SNARE') { $drum = 2 }
        elseif ($entry.family -eq 'HAT_CLOSED') { $drum = 3 }

        $directoryWriter.Write([uint32]$dataOffset)
        $directoryWriter.Write([uint32]$wave.Pcm.Length)
        $directoryWriter.Write($loopStart)
        $directoryWriter.Write($loopLength)
        $directoryWriter.Write([uint32]$wave.Rate)
        $directoryWriter.Write([uint32][Math]::Round($rootHz * 65536.0))
        $directoryWriter.Write($flags)
        $directoryWriter.Write([byte]$familyIds[$entry.family])
        $directoryWriter.Write([byte]0)
        $directoryWriter.Write([byte][Math]::Min(64, [Math]::Round([double]$entry.gain * 64.0)))
        $directoryWriter.Write([byte]128)
        $directoryWriter.Write([byte]$waveMasks[$entry.preferred_waveform])
        $directoryWriter.Write($drum)
        Write-FixedAscii $directoryWriter ([IO.Path]::GetFileNameWithoutExtension($entry.file)) 16
        for ($i = 0; $i -lt 16; ++$i) { $directoryWriter.Write([byte]0) }
    }
    while ($directoryStream.Length -lt $EntrySize * $DirectoryEntries) { $directoryWriter.Write([byte]0) }
    while (($pcmStream.Length -band 3) -ne 0) { $pcmWriter.Write([byte]0) }

    $directory = $directoryStream.ToArray()
    $pcm = $pcmStream.ToArray()
    $totalBytes = $PcmOffset + $pcm.Length
    if ($totalBytes -gt 384KB) { throw "Bank is too large: $totalBytes bytes" }

    $output = [IO.File]::Open($OutputPath, [IO.FileMode]::Create, [IO.FileAccess]::Write)
    $writer = New-Object IO.BinaryWriter($output)
    try {
        Write-FixedAscii $writer "KFSAMP1`0" 8
        $writer.Write([uint16]1)
        $writer.Write([uint16]$HeaderSize)
        $writer.Write([uint32]$Generation)
        $writer.Write([uint32]$totalBytes)
        $writer.Write([uint16]$manifest.Count)
        $writer.Write([uint16]$EntrySize)
        $writer.Write([uint32]$DirectoryOffset)
        $writer.Write([uint32]$PcmOffset)
        $writer.Write([uint32](Get-Crc32 $directory))
        $writer.Write([uint32](Get-Crc32 $pcm))
        $writer.Write([uint32]0)
        Write-FixedAscii $writer $BankName 16
        $writer.Write($CommitMarker)
        $writer.Write($directory)
        while ($output.Position -lt $PcmOffset) { $writer.Write([byte]0) }
        $writer.Write($pcm)
    } finally { $writer.Dispose(); $output.Dispose() }
} finally {
    $directoryWriter.Dispose(); $directoryStream.Dispose()
    $pcmWriter.Dispose(); $pcmStream.Dispose()
}

Write-Host "Built $OutputPath ($totalBytes bytes, $($manifest.Count) instruments)"
