#!/usr/bin/env bash

set -euo pipefail

script_path=$(cd -P -- "$(dirname -- "$0")" && pwd -P)
. "${script_path}/shell_include.sh"

if ! command -v grub-mkrescue >/dev/null 2>&1; then
    die "grub-mkrescue is required to build the installer ISO"
fi

if [ ! -f "Root/boot/Kernel" ]; then
    die "Root/boot/Kernel was not found. Run 'ninja install' first."
fi

if [ ! -d "Root" ]; then
    die "Root directory missing. Run 'ninja install' first."
fi

work_dir=$(mktemp -d)
iso_root="${work_dir}/iso_root"
mkdir -p "${iso_root}/boot/grub"

cp "Root/boot/Kernel" "${iso_root}/boot/Kernel"
cp -r "Root" "${iso_root}/live_root"

cat > "${iso_root}/boot/grub/grub.cfg" <<EOF
set timeout=3
set default=0

menuentry "MixUpOS Installer ISO (GUI)" {
    multiboot /boot/Kernel root=lun2:0:0
}

menuentry "MixUpOS Installer ISO (text mode)" {
    multiboot /boot/Kernel graphics_subsystem_mode=off root=lun2:0:0
}
EOF

output_iso="${PWD}/mixupos-installer.iso"
grub-mkrescue -o "${output_iso}" "${iso_root}" >/dev/null 2>&1
echo "Created installer ISO: ${output_iso}"

rm -rf "${work_dir}"
