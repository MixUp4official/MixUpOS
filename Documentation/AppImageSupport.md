# MixUpOS Experimental AppImage Support

MixUpOS Rebranding Edition currently provides **experimental** AppImage support for host workflows.

## What this means

- This is a helper for running Linux AppImages on the **host system** during development.
- It is **not** a full in-OS compatibility layer yet.
- Compatibility depends on the AppImage itself and your host libraries.

## Runner script

Use:

```bash
./Toolchain/MixUpAppImageRunner.sh /path/to/SomeApp.AppImage
```

Pass additional arguments after the AppImage path:

```bash
./Toolchain/MixUpAppImageRunner.sh /path/to/SomeApp.AppImage --flag value
```

## Behavior

The script attempts two strategies:

1. `--appimage-extract-and-run` (preferred, if supported)
2. fallback extraction via `--appimage-extract` and launch of `squashfs-root/AppRun`

## Limitations

- Some AppImages may still fail due to missing host dependencies.
- Sandboxing integrations are app-dependent.
- GUI integration (desktop entries, icon install, update channel) is not handled yet.

## Roadmap ideas

- Add a userland `AppImageManager` utility.
- Desktop integration and launcher registration.
- Signature / integrity validation for downloaded AppImages.
