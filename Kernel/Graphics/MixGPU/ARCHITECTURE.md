# MixGPU Architecture Blueprint

This document defines a staged architecture for a future in-tree MixGPU stack.

## 1) Layering

1. **Kernel device layer** (`Kernel/Devices/GPU/MixGPU/...`)
   - Discovers hardware-backed (or virtual) MixGPU devices.
   - Exposes one canonical device contract to higher kernel graphics services.
2. **Kernel graphics adapter layer** (`Kernel/Graphics/MixGPU/...`)
   - Implements API-focused adapters (OpenGL and Vulkan-facing responsibilities).
   - Keeps policy (feature flags, fallback rules) out of device probing.
3. **Userspace runtime layer** (`/usr/lib/libGL.so*`, `/usr/lib/libvulkan.so*`)
   - Provides graphics loader/runtime libraries.
4. **Profile/diagnostics layer** (`/usr/share/graphics/*.profile`, DriverHub/AppStudio)
   - Surfaces readiness and compatibility state to users.

## 2) Core principles

- **Single source of truth** for runtime probe paths.
- **No fake success states**: diagnostics must show which artifact was found.
- **Predictable fallback order**: runtime library first, profile fallback second.
- **Isolation of responsibilities**: kernel/device concerns stay out of GUI code.

## 3) Planned interfaces

- `MixGPUDevice`
  - Lifetime/device ownership.
  - Capability query (renderer generation, queue counts, memory classes).
- `MixGPUOpenGLAdapter`
  - OpenGL-focused command submission orchestration.
- `MixGPUVulkanAdapter`
  - Vulkan queue/ICD handshake orchestration.
- `MixGPUProbePolicy`
  - Shared readiness probe policy used by diagnostics UIs.

## 4) Rollout phases

1. **Phase A (current)**: profile + diagnostics contracts, no production kernel driver.
2. **Phase B**: kernel device skeleton + capability reporting.
3. **Phase C**: graphics adapter classes with command-path stubs.
4. **Phase D**: hardware-backed implementation and performance validation.

## 5) Non-goals for this phase

- Full Windows driver model parity.
- Shipping production GPU kernel modules from this folder in the current milestone.
