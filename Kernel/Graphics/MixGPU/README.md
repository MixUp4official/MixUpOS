# MixGPU (Experimental)

This folder tracks the architecture for a self-managed graphics stack in MixUpOS.

## What exists today

- Userspace profile mapping for OpenGL/Vulkan runtimes.
- App diagnostics integration in AppStudio and DriverHub.
- Architecture blueprint for evolving toward real kernel-side adapters.

## Kernel target layout (planned)

- `Kernel/Devices/GPU/MixGPU/` for device discovery and base contracts.
- `Kernel/Graphics/MixGPU/` for API adapters and probe policy.

## Runtime contract

- OpenGL runtime artifacts:
  - `/usr/lib/libGL.so`
  - `/usr/lib/libGL.so.1`
  - `/usr/share/graphics/mixgpu-opengl.profile`
- Vulkan runtime artifacts:
  - `/usr/lib/libvulkan.so`
  - `/usr/lib/libvulkan.so.1`
  - `/usr/share/graphics/mixgpu-vulkan.profile`

## Next step

See `ARCHITECTURE.md` for layering, rollout phases, and interface boundaries.
