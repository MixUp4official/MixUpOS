# /linux-driver

Place Linux driver compatibility manifests in this folder.

A manifest maps Linux PCI IDs to an existing native MixUpOS driver path.

## Manifest format (`*.ldriver`)

```ini
name=Intel I219-V
pci_ids=8086:15bc,8086:15be
linux_driver=e1000e
native_driver=Kernel/Net/Intel/E1000ENetworkAdapter
status=enabled
```

## Workflow

1. Drop/create `*.ldriver` files in `/linux-driver`.
2. Open **LinuxDriverManager**.
3. Click **Rescan** to validate manifests.
4. Use output to match Linux IDs to existing native drivers.

This provides real Linux-driver compatibility mapping at the hardware ID layer without needing proprietary binary modules.

## Preinstalled graphics mappings

MixUpOS ships with graphics compatibility manifests for:

- OpenGL userspace/runtime mapping
- Vulkan ICD/loader mapping

These manifests are provided in `/linux-driver/examples` and can be used as a baseline for self-managed driver bring-up in `/kernel` and `/base`.


For kernel-side planning, see `Kernel/Graphics/MixGPU/ARCHITECTURE.md` (layering, rollout phases, and adapter boundaries).
