param(
    [string]$ManifestPath = (Join-Path $PSScriptRoot 'AMIGA_SAMPLE_CANDIDATES.csv'),
    [string]$OutputPath = (Join-Path $PSScriptRoot 'audition_64'),
    [int]$RawPreviewRate = 8287
)

$ErrorActionPreference = 'Stop'

function Read-BigEndianUInt32([byte[]]$Data, [int]$Offset)
{
    return ([uint32]$Data[$Offset] * 16777216) +
           ([uint32]$Data[$Offset + 1] * 65536) +
           ([uint32]$Data[$Offset + 2] * 256) +
           [uint32]$Data[$Offset + 3]
}

function Get-8SvxBody([byte[]]$Data)
{
    if ($Data.Length -lt 12 -or
        [Text.Encoding]::ASCII.GetString($Data, 0, 4) -ne 'FORM' -or
        [Text.Encoding]::ASCII.GetString($Data, 8, 4) -ne '8SVX')
    {
        throw 'File is not an IFF/8SVX sample.'
    }

    $offset = 12
    while ($offset + 8 -le $Data.Length)
    {
        $tag = [Text.Encoding]::ASCII.GetString($Data, $offset, 4)
        $length = Read-BigEndianUInt32 $Data ($offset + 4)
        $dataOffset = $offset + 8
        if ($dataOffset + $length -gt $Data.Length)
        {
            throw "Invalid $tag chunk length $length."
        }

        if ($tag -eq 'BODY')
        {
            $body = New-Object byte[] $length
            [Array]::Copy($Data, $dataOffset, $body, 0, $length)
            return $body
        }
        $offset = $dataOffset + $length + ($length % 2)
    }
    throw 'IFF/8SVX BODY chunk was not found.'
}

function Write-Pcm8Wave(
    [string]$Path,
    [byte[]]$SignedPcm,
    [int]$SampleRate)
{
    # Amiga PCM is signed 8-bit; WAV PCM stores 8-bit samples as unsigned.
    $wavePcm = New-Object byte[] $SignedPcm.Length
    for ($i = 0; $i -lt $SignedPcm.Length; ++$i)
    {
        $wavePcm[$i] = $SignedPcm[$i] -bxor 0x80
    }

    $stream = [IO.File]::Open($Path, [IO.FileMode]::Create,
                              [IO.FileAccess]::Write, [IO.FileShare]::None)
    try
    {
        $writer = New-Object IO.BinaryWriter($stream)
        try
        {
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
            $writer.Write([int]$wavePcm.Length)
            $writer.Write($wavePcm)
        }
        finally
        {
            $writer.Dispose()
        }
    }
    finally
    {
        $stream.Dispose()
    }
}

$manifest = Import-Csv -LiteralPath $ManifestPath
New-Item -ItemType Directory -Path $OutputPath -Force | Out-Null

foreach ($entry in $manifest)
{
    $sourcePath = Join-Path $PSScriptRoot $entry.path
    $sourceData = [IO.File]::ReadAllBytes($sourcePath)
    if ($entry.container -eq '8SVX')
    {
        $pcm = Get-8SvxBody $sourceData
        $rate = [int]$entry.native_rate_hz
    }
    else
    {
        $pcm = $sourceData
        $rate = $RawPreviewRate
    }

    if ($pcm.Length -ne [int]$entry.pcm_bytes)
    {
        throw "PCM length mismatch for candidate $($entry.id): $($entry.file)"
    }

    $safeName = $entry.file -replace '[^A-Za-z0-9_.()-]', '_'
    $waveName = '{0}_{1}_{2}.wav' -f $entry.id, $entry.pack, $safeName
    Write-Pcm8Wave (Join-Path $OutputPath $waveName) $pcm $rate
}

Write-Host "Generated $($manifest.Count) WAV previews in $OutputPath"
