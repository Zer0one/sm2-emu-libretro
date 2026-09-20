#Requires -RunAsAdministrator

[CmdletBinding()]
param(
    [string]$TestRoot = (Join-Path $env:USERPROFILE "Codex\sm2-emu-libretro-tests")
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$CodexPublicKey = "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIFSkVwsWou+rrUTix0bGEduXx72GgOxXMDUmSIV6/67L codex-windows-test-host"
$OpenSshCapability = "OpenSSH.Server~~~~0.0.1.0"
$FirewallRule = "OpenSSH-Server-In-TCP"
$LogDirectory = if ($PSScriptRoot -and (Test-Path -LiteralPath $PSScriptRoot)) {
    $PSScriptRoot
}
else {
    $env:TEMP
}
$LogPath = Join-Path $LogDirectory ("sm2-windows-host-setup-{0}.log" -f (Get-Date -Format "yyyyMMdd-HHmmss"))

function Write-Step {
    param([string]$Message)
    Write-Host "`n==> $Message" -ForegroundColor Cyan
}

function Install-OpenSshServer {
    if (Get-Service -Name sshd -ErrorAction SilentlyContinue) {
        Write-Host "OpenSSH Server is already installed."
        return
    }

    Write-Step "Installing OpenSSH Server"
    $process = Start-Process -FilePath "$env:SystemRoot\System32\dism.exe" `
        -ArgumentList "/Online", "/Add-Capability", "/CapabilityName:$OpenSshCapability" `
        -Wait -PassThru -NoNewWindow

    if ($process.ExitCode -notin 0, 3010) {
        throw "DISM could not install OpenSSH Server (exit code $($process.ExitCode))."
    }

    if (-not (Get-Service -Name sshd -ErrorAction SilentlyContinue)) {
        throw "OpenSSH installation completed, but the sshd service was not found. Restart Windows and run this script again."
    }

    if ($process.ExitCode -eq 3010) {
        Write-Warning "Windows requested a restart. Complete this script, restart Windows, and run it once more."
    }
}

function Write-SystemSummary {
    Write-Step "Recording the Windows host"
    $windows = Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion"
    Write-Host "Windows product  : $($windows.ProductName)"
    Write-Host "Display version  : $($windows.DisplayVersion)"
    Write-Host "Build            : $($windows.CurrentBuild).$($windows.UBR)"
    Write-Host "PowerShell       : $($PSVersionTable.PSVersion) ($($PSVersionTable.PSEdition))"
    Write-Host "Account          : $([Security.Principal.WindowsIdentity]::GetCurrent().Name)"
    Write-Host "Script           : $PSCommandPath"
    Write-Host "Log              : $LogPath"
}

function Configure-SshService {
    Write-Step "Configuring the SSH service"
    Set-Service -Name sshd -StartupType Automatic
    Start-Service -Name sshd
}

function Configure-SshFirewall {
    Write-Step "Restricting SSH to the private local network"
    $rule = Get-NetFirewallRule -Name $FirewallRule -ErrorAction SilentlyContinue

    if (-not $rule) {
        New-NetFirewallRule `
            -Name $FirewallRule `
            -DisplayName "OpenSSH Server (sshd)" `
            -Enabled True `
            -Direction Inbound `
            -Protocol TCP `
            -Action Allow `
            -LocalPort 22 `
            -Profile Private `
            -RemoteAddress LocalSubnet | Out-Null
    }
    else {
        Set-NetFirewallRule -Name $FirewallRule -Enabled True -Profile Private | Out-Null
        Get-NetFirewallRule -Name $FirewallRule |
            Get-NetFirewallAddressFilter |
            Set-NetFirewallAddressFilter -RemoteAddress LocalSubnet | Out-Null
    }
}

function Add-AuthorizedKey {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = [Security.Principal.WindowsPrincipal]::new($identity)
    $isAdministrator = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

    if ($isAdministrator) {
        $authorizedKeys = Join-Path $env:ProgramData "ssh\administrators_authorized_keys"
    }
    else {
        $sshDirectory = Join-Path $env:USERPROFILE ".ssh"
        New-Item -ItemType Directory -Path $sshDirectory -Force | Out-Null
        $authorizedKeys = Join-Path $sshDirectory "authorized_keys"
    }

    Write-Step "Authorizing the dedicated Codex SSH key"

    $keys = @()
    if (Test-Path $authorizedKeys) {
        $keys = @(Get-Content -LiteralPath $authorizedKeys | Where-Object { $_.Trim() })
    }

    if ($keys -notcontains $CodexPublicKey) {
        $keys += $CodexPublicKey
        $contents = ($keys -join [Environment]::NewLine) + [Environment]::NewLine
        [IO.File]::WriteAllText($authorizedKeys, $contents, [Text.Encoding]::ASCII)
    }

    if ($isAdministrator) {
        & icacls.exe $authorizedKeys /inheritance:r /grant:r "*S-1-5-32-544:F" "*S-1-5-18:F" | Out-Null
    }
    else {
        $currentUserSid = $identity.User.Value
        & icacls.exe $authorizedKeys /inheritance:r /grant:r "*$($currentUserSid):F" "*S-1-5-18:F" | Out-Null
    }

    if ($LASTEXITCODE -ne 0) {
        throw "Could not set safe permissions on $authorizedKeys."
    }

    return [PSCustomObject]@{
        Path = $authorizedKeys
        IsAdministrator = $isAdministrator
    }
}

function Initialize-TestRoot {
    Write-Step "Creating the isolated test workspace"
    foreach ($directory in @(
        $TestRoot,
        (Join-Path $TestRoot "incoming"),
        (Join-Path $TestRoot "runs"),
        (Join-Path $TestRoot "results"),
        (Join-Path $TestRoot "tools")
    )) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }

    $marker = Join-Path $TestRoot ".codex-sm2-test-root"
    [IO.File]::WriteAllText($marker, "SM2-Emu Libretro isolated Windows test root`r`n", [Text.Encoding]::ASCII)
}

function Show-NetworkSummary {
    $activeProfiles = @(Get-NetConnectionProfile | Where-Object {
        $_.IPv4Connectivity -ne "Disconnected" -or $_.IPv6Connectivity -ne "Disconnected"
    })
    $addresses = @(Get-NetIPAddress -AddressFamily IPv4 | Where-Object {
        $_.IPAddress -notlike "127.*" -and
        $_.IPAddress -notlike "169.254.*" -and
        $_.AddressState -eq "Preferred"
    })

    Write-Step "Setup summary"
    Write-Host "Windows account : $env:USERNAME"
    Write-Host "Computer name   : $env:COMPUTERNAME"
    Write-Host "Test root       : $TestRoot"
    Write-Host "SSH service     : $((Get-Service sshd).Status) / Automatic"
    Write-Host "SSH key file    : $($script:KeyResult.Path)"
    Write-Host "Administrator   : $($script:KeyResult.IsAdministrator)"

    Write-Host "`nActive network profiles:"
    $activeProfiles | Format-Table InterfaceAlias, Name, NetworkCategory, IPv4Connectivity -AutoSize

    Write-Host "IPv4 addresses:"
    $addresses | Format-Table InterfaceAlias, IPAddress -AutoSize

    if ($activeProfiles.NetworkCategory -notcontains "Private") {
        Write-Warning "No active network is marked Private. SSH remains blocked until the trusted local network is changed to Private in Windows Settings."
    }

    Write-Host "Keep this Windows account signed in and the PC awake during graphical RetroArch tests." -ForegroundColor Yellow
    Write-Host "Send the Windows account name and the LAN IPv4 address shown above to Codex." -ForegroundColor Green
}

function Write-FinalDiagnostics {
    Write-Step "Final diagnostics"
    Get-Service -Name sshd | Format-List Name, Status, StartType
    Get-NetFirewallRule -Name $FirewallRule |
        Format-List Name, Enabled, Direction, Action, Profile
    Get-NetFirewallRule -Name $FirewallRule |
        Get-NetFirewallAddressFilter |
        Format-List RemoteAddress
    $listeners = @(Get-NetTCPConnection -LocalPort 22 -State Listen -ErrorAction SilentlyContinue)
    $listeners | Format-Table LocalAddress, LocalPort, State, OwningProcess -AutoSize
    if (-not $listeners) {
        throw "sshd is running but no process is listening on TCP port 22."
    }

    $sshd = Join-Path $env:SystemRoot "System32\OpenSSH\sshd.exe"
    if (Test-Path -LiteralPath $sshd) {
        & $sshd -t
        Write-Host "sshd configuration check exit code: $LASTEXITCODE"
        if ($LASTEXITCODE -ne 0) {
            throw "sshd rejected its current configuration."
        }
    }

    try {
        Get-CimInstance Win32_VideoController |
            Select-Object Name, DriverVersion, AdapterRAM |
            Format-Table -AutoSize
    }
    catch {
        Write-Warning "GPU inventory unavailable: $($_.Exception.Message)"
    }
}

$exitCode = 0
$transcriptStarted = $false

try {
    Start-Transcript -LiteralPath $LogPath -Force | Out-Null
    $transcriptStarted = $true

    Write-SystemSummary
    Install-OpenSshServer
    Configure-SshService
    Configure-SshFirewall
    $script:KeyResult = Add-AuthorizedKey
    Initialize-TestRoot
    Show-NetworkSummary
    Write-FinalDiagnostics

    Write-Host "`nWindows test host setup completed successfully." -ForegroundColor Green
}
catch {
    $exitCode = 1
    Write-Host "`nSETUP FAILED" -ForegroundColor Red
    Write-Host "Message          : $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "Category         : $($_.CategoryInfo.Category)" -ForegroundColor Red
    Write-Host "Command          : $($_.InvocationInfo.MyCommand)" -ForegroundColor Red
    Write-Host "Position         : $($_.InvocationInfo.PositionMessage)" -ForegroundColor Red
    if ($_.ScriptStackTrace) {
        Write-Host "Script stack     : $($_.ScriptStackTrace)" -ForegroundColor Red
    }
}
finally {
    if ($transcriptStarted) {
        Stop-Transcript | Out-Null
    }
    Write-Host "`nSetup log: $LogPath" -ForegroundColor Yellow
}

exit $exitCode
