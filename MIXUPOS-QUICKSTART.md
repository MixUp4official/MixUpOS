# MixUpOS Quickstart

MixUpOS is a SerenityOS-based fork prepared in this folder with MixUpOS branding and boot configuration names.

## Windows quick start

1. Install a WSL2 distro:

```powershell
.\setup-mixupos-wsl.ps1
```

2. Start the distro once, create your Linux user, and install the packages listed in [Documentation/BuildInstructions.md](Documentation/BuildInstructions.md).

3. Build and boot MixUpOS from this folder:

```powershell
.\boot-mixupos.ps1
```

## Direct build command

Inside Linux/WSL you can also run:

```bash
bash Meta/mixupos.sh run
```

## Notes

- The upstream build system still uses internal `SERENITY_*` variable names. That is intentional to keep the base stable.
- If QEMU is missing on Windows/WSL, follow [Documentation/BuildInstructionsWindows.md](Documentation/BuildInstructionsWindows.md).
