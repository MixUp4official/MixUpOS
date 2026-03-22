Write-Host "Installiere eine WSL2-Distribution fuer MixUpOS..."
Write-Host "Falls Windows einen Neustart verlangt, danach dieses Repo erneut oeffnen."

& wsl.exe --install Ubuntu --no-launch
if ($LASTEXITCODE -ne 0) {
    Write-Host "Die WSL-Installation konnte nicht abgeschlossen werden."
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "WSL-Setup angestossen."
Write-Host "Naechste Schritte:"
Write-Host "1. Ubuntu einmal starten und den Linux-Benutzer anlegen."
Write-Host "2. Die Abhaengigkeiten aus Documentation/BuildInstructions.md installieren."
Write-Host "3. Danach .\boot-mixupos.ps1 ausfuehren."
