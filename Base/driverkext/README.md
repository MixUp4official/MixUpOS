# /driverkext

Drop macOS `.kext` bundles in this directory.

MixUpOS scans this folder via `KextManager` and lets you inspect bundles with `CompatibilityAssistant`.

Current support level:
- Bundle discovery and metadata inspection
- CFBundleExecutable Mach-O header validation
- File-level compatibility flow
- No direct binary execution of macOS kernel extensions
