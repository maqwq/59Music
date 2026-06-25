try {
    $s = Invoke-RestMethod -Uri 'http://localhost:1059/api/v1/player/state' -UseBasicParsing
    Write-Host $s.data.isPlaying
    Write-Host $s.data.currentPosition
} catch {
    Write-Host "dead"
}
