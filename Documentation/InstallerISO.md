# MixUpOS installer ISO (real hardware)

MixUpOS now ships a dedicated installer ISO generation target focused on **real x86_64 hardware** testing and installation workflows.

## Build the installer ISO

From your build directory:

```sh
ninja install
ninja installer-iso-image
```

This generates:

- `mixupos-installer.iso`

## Write ISO to USB

On Linux:

```sh
sudo dd if=mixupos-installer.iso of=/dev/sdX bs=4M status=progress oflag=sync
```

Replace `/dev/sdX` with your USB device. This command destroys all data on that device.

## Installation workflow (GUI-oriented)

1. Boot from the USB on target hardware (UEFI or BIOS via GRUB ISO entry).
2. Launch **Recovery Center** from the desktop.
3. Use the hardware audit and installer guidance tools to verify driver readiness.
4. Use `PartitionEditor` to prepare the destination disk.
5. Deploy the prepared image (`grub_uefi_disk_image` or `grub_disk_image`) to the selected disk.

## Driver readiness goals for fair Windows comparisons

For practical comparisons against Windows on the same machine, validate:

- Wired networking (e1000/e1000e/rtl8168-compatible NICs)
- Framebuffer/GPU stability and desktop compositing
- Input stack latency (keyboard/mouse/gamepad)
- Audio output under sustained load

Use **DriverHub** for module health and compatibility checks before benchmarking.
