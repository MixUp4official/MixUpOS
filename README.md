# MixUpOS — Rebranded Desktop OS

```text
 __  __ _       _   _       ___  ____
|  \/  (_)_  _ | | | |_ __ / _ \/ ___|
| |\/| | \ \/ /| | | | '_ \ | | \___ \
| |  | | |>  < | | | |_) | |_| |__) |
|_|  |_|_/_/\_\|_|_|_| .__/ \___/____/
                     |_|
```

**MixUpOS** is a fully branded, graphical Unix-like desktop OS with a strong built-in productivity and development workflow.

## Highlights

- Consistent MixUpOS branding and UX
- Full desktop stack (window server, taskbar, apps, services)
- 64-bit targets: x86_64, AArch64, RISC-V
- Browser/web stack, POSIX-like tooling, and ports ecosystem

## Preinstalled Apps (Major Upgrade)

MixUpOS now ships with a stronger default app set:

- **CodeIt**: full integrated developer IDE
- **MixOffice**: unified productivity hub
  - Writer (TextEditor)
  - Spreadsheet (Spreadsheet)
  - Presentation (Presenter)
  - Quick-launch for CodeIt, Python 3 REPL, Terminal, FileManager, Browser
- **Python 3 REPL**: real CPython runtime from `/usr/local/bin/python3`
- **AppImage Launcher**: GUI flow for opening `.AppImage` packages
- **KextManager**: scans `/driverkext` for macOS `.kext` bundles and opens compatibility workflows
- **LinuxDriverManager**: validates and manages Linux compatibility manifests in `/linux-driver`
- **FocusTimer**: Pomodoro-style custom focus timer
- **DevLaunchpad**: quick launcher for CodeIt, Terminal, FileManager, Browser

## Real Driver Support (Including Linux-Class Hardware Coverage)

MixUpOS uses native drivers and supports a broad set of hardware families that are commonly supported by Linux-class ecosystems, including:

- Intel e1000/e1000e NIC families (native kernel drivers)
- Realtek RTL8168 NIC support (native kernel driver)
- Intel HDA audio support (native kernel driver)
- Intel and AMD CPU profile detection, including AMD Ryzen profile selection at boot

This is **real native driver support** in the kernel (not just UI wrappers).

## macOS `.kext` Drop-Folder Workflow

You can now drop `.kext` bundles into:

```text
/driverkext
```

Then open **KextManager** to scan bundles and inspect them through `CompatibilityAssistant`.
KextManager also validates Mach-O executable headers (`CFBundleExecutable`) so dropped bundles are checked automatically.
This provides a practical macOS-driver compatibility workflow at the bundle-management layer.

## Linux Driver Support (`/linux-driver`)

You can place Linux compatibility manifests in:

```text
/linux-driver
```

Then open **LinuxDriverManager** and click **Rescan**.
Each `*.ldriver` file maps Linux PCI IDs (for example from upstream Linux drivers) to existing native MixUpOS drivers.

Example manifest:

```ini
name=Intel I219 Family
pci_ids=8086:15bc,8086:15be,8086:15e3
linux_driver=e1000e
native_driver=Kernel/Net/Intel/E1000ENetworkAdapter
status=enabled
```

### GPU/Display Driver Improvements

Recent kernel improvements include better generic display fallback behavior:

- Boot framebuffer detection now supports both **BGRx8888** and **RGBx8888** pixel layouts.
- Generic framebuffer console colors are adapted for RGBx framebuffers.
- Device-tree simple-framebuffer parsing now accepts both `x8r8g8b8/a8r8g8b8` and `x8b8g8r8/a8b8g8r8`.
- Additional simple-framebuffer aliases now include `r8g8b8x8` and `b8g8r8x8` for broader self-made/custom firmware setups.

This improves practical compatibility for modern laptops (for example AMD-based systems similar to Lenovo V14 AMN G4) when no vendor-native GPU driver is available yet.

For Lenovo V14 AMN G4-like hardware, this means improved chance to boot and render correctly via framebuffer fallback even before a full vendor-native GPU stack is available.

## Office and File Routing

`MixOffice` can route these document formats automatically:

- Documents: `doc`, `docx`, `odt`, `rtf`, `txt`, `md`
- Sheets: `xls`, `xlsx`, `ods`, `csv`
- Presentations: `ppt`, `pptx`, `odp`

## Quick Start

```bash
git clone <repo-url>
cd MixUpOS
```

Then continue with:

- Build guide: [`Documentation/BuildInstructions.md`](Documentation/BuildInstructions.md)
- Ladybird build: [`Documentation/BuildInstructionsLadybird.md`](Documentation/BuildInstructionsLadybird.md)
- Quickstart: [`MIXUPOS-QUICKSTART.md`](MIXUPOS-QUICKSTART.md)

## Python Toolchain Build (Host)

```bash
./Toolchain/BuildPython.sh
```

## AppImage (Experimental)

```bash
./Toolchain/MixUpAppImageRunner.sh /path/to/App.AppImage
```

See: [`Documentation/AppImageSupport.md`](Documentation/AppImageSupport.md)

## Project Layout

```text
Kernel/        Kernel and drivers
Userland/      Applications, libraries, services
Base/          Themes, icons, manuals, resources
Documentation/ Project documentation
Toolchain/     Build/toolchain scripts
Ports/         Ported software
```

## Contributing

- Contribution guide: [`CONTRIBUTING.md`](CONTRIBUTING.md)
- Documentation: [`Documentation/`](Documentation/)

## License

BSD-2-Clause (see [`LICENSE`](LICENSE)).
