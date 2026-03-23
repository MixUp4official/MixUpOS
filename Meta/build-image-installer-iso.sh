#!/usr/bin/env bash

set -euo pipefail

script_path=$(cd -P -- "$(dirname -- "$0")" && pwd -P)
. "${script_path}/shell_include.sh"

usage() {
    cat <<'USAGE'
Usage: Meta/build-image-installer-iso.sh [--output PATH] [--label LABEL] [--keep-work-dir]

Builds a bootable MixUpOS installer ISO that bundles the current Root tree.

Options:
  --output PATH    Output ISO path (default: ./mixupos-installer.iso)
  --label LABEL    ISO9660 volume label (default: MIXUPOS_INSTALLER)
  --keep-work-dir  Do not delete the generated temporary ISO staging directory
  -h, --help       Show this help
USAGE
}

output_iso="${PWD}/mixupos-installer.iso"
iso_label="MIXUPOS_INSTALLER"
keep_work_dir=0

while [ $# -gt 0 ]; do
    case "$1" in
        --output)
            [ $# -ge 2 ] || die "--output requires a path argument"
            output_iso="$2"
            shift 2
            ;;
        --label)
            [ $# -ge 2 ] || die "--label requires a label argument"
            iso_label="$2"
            shift 2
            ;;
        --keep-work-dir)
            keep_work_dir=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "Unknown argument: $1"
            ;;
    esac
done

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

cleanup() {
    if [ "$keep_work_dir" -eq 1 ]; then
        echo "Keeping temporary work directory: ${work_dir}"
        return
    fi
    rm -rf "${work_dir}"
}
trap cleanup EXIT

mkdir -p "${iso_root}/boot/grub"

cp "Root/boot/Kernel" "${iso_root}/boot/Kernel"
cp -r "Root" "${iso_root}/live_root"

cat > "${iso_root}/boot/grub/grub.cfg" <<EOF_GRUB
set timeout=5
set default=0

menuentry "MixUpOS Installer (GUI)" {
    multiboot /boot/Kernel root=lun2:0:0
}

menuentry "MixUpOS Installer (text mode)" {
    multiboot /boot/Kernel graphics_subsystem_mode=off root=lun2:0:0
}

menuentry "MixUpOS Installer (safe mode)" {
    multiboot /boot/Kernel disable_virtio root=lun2:0:0
}
EOF_GRUB

mkdir -p "$(dirname -- "${output_iso}")"

echo "Creating installer ISO..."
echo "  Output: ${output_iso}"
echo "  Label:  ${iso_label}"

grub-mkrescue -volid "${iso_label}" -o "${output_iso}" "${iso_root}"

echo "Created installer ISO: ${output_iso}"
