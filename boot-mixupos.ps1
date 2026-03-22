param(
    [ValidateSet("run", "build", "image", "test", "gdb", "help")]
    [string]$Command = "run",
    [ValidateSet("x86_64", "aarch64", "riscv64", "lagom")]
    [string]$Target = "x86_64",
    [ValidateSet("GNU", "Clang")]
    [string]$Toolchain = "GNU",
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$ExtraArgs
)

function Quote-BashArgument {
    param([string]$Value)
    $escapedSingleQuote = "'""'""'"
    return "'" + $Value.Replace("'", $escapedSingleQuote) + "'"
}

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$installedDistros = & wsl.exe -l -q 2>$null | ForEach-Object { $_.Trim() } | Where-Object { $_ }

if (-not $installedDistros) {
    Write-Host "Keine WSL-Distribution installiert."
    Write-Host "Starte zuerst .\setup-mixupos-wsl.ps1 oder manuell 'wsl --install Ubuntu --no-launch'."
    exit 1
}

$commandParts = @("bash", "./Meta/mixupos.sh", $Command)
if ($Command -ne "help") {
    $commandParts += $Target
    if ($Target -ne "lagom") {
        $commandParts += $Toolchain
    }
}
if ($ExtraArgs) {
    $commandParts += $ExtraArgs
}

$bashCommand = ($commandParts | ForEach-Object { Quote-BashArgument $_ }) -join " "

Write-Host "Starte MixUpOS ueber WSL im Ordner $repoRoot"
& wsl.exe --cd $repoRoot --exec bash -lc $bashCommand
exit $LASTEXITCODE
