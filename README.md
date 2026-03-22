# MixUpOS — Rebranded Desktop OS

**MixUpOS** ist ein vollständig eigenständig gebrandetes, grafisches Unix-ähnliches Betriebssystem auf SerenityOS-Technikbasis.

## 100% Rebranding-Ziele

- Einheitliche Produktidentität: **MixUpOS** statt gemischter Altbezeichnungen
- Modernes Desktop-Look-and-Feel mit konsistenter Theme-Sprache
- Eigene Developer-Experience inkl. klarer Build- und Tooling-Wege

## MixUpOS Kernfunktionen

- 64-Bit-System für x86_64 / AArch64 / RISC-V
- Voller GUI-Desktop mit WindowServer, Taskbar, Apps und Services
- Browser/Web-Stack, POSIX-nahe Userland-Werkzeuge, Ports-Ökosystem
- Konfigurierbare Themes und Color-Schemes

## MixUpOS Schnellstart

```bash
git clone <repo-url>
cd MixUpOS
```

Danach:

- Build-Anleitung: [`Documentation/BuildInstructions.md`](Documentation/BuildInstructions.md)
- Ladybird-Build: [`Documentation/BuildInstructionsLadybird.md`](Documentation/BuildInstructionsLadybird.md)
- Quickstart: [`MIXUPOS-QUICKSTART.md`](MIXUPOS-QUICKSTART.md)

## MixUpOS Experimentale AppImage-Unterstützung

MixUpOS liefert einen ersten Host-Runner für AppImages:

```bash
./Toolchain/MixUpAppImageRunner.sh /pfad/zu/Programm.AppImage
```

Details und Einschränkungen: [`Documentation/AppImageSupport.md`](Documentation/AppImageSupport.md)

## MixUpOS Projektstruktur

```text
Kernel/        Kernel und Treiber
Userland/      Anwendungen, Libraries, Services
Base/          Ressourcen (Themes, Icons, Manpages)
Documentation/ Projektdokumentation
Toolchain/     Build/Toolchain-Skripte
Ports/         Portierte Software
```

## MixUpOS Mitmachen

- Contribution Guide: [`CONTRIBUTING.md`](CONTRIBUTING.md)
- Dokumentation: [`Documentation/`](Documentation/)

## Lizenz

BSD-2-Clause (siehe [`LICENSE`](LICENSE)).
