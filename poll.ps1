for ($i=0; $i -lt 5; $i++) {
    Start-Sleep -Seconds 1
    try {
        $s = Invoke-RestMethod -Uri 'http://localhost:1059/api/v1/player/state' -UseBasicParsing
        Write-Host "tick $($i+1): playing=$($s.data.isPlaying) pos=$($s.data.currentPosition)"
    } catch {
        Write-Host "tick $($i+1): 服务器已关闭"
        break
    }
}
