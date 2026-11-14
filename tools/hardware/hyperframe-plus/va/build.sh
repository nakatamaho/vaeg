#!/bin/sh
# Build HFV1.COM and a bootable MS-DOS 4.0 test disk (M149):
#   build.sh [--msdos4 msdos4-pc88va-2hd.d88] OUTPUT.d88
# The MS-DOS 4.0 image defaults to the shared cache
# ($VAEG_CACHE_DIR/msdos4-va.2/dl/msdos4-pc88va-2hd.d88). The disk stays
# outside the repository. The build id is the first 8 hex digits of the
# checkout's HEAD (BUILD_ID overrides).
set -eu

nasm_command=${NASM:-nasm}
python_command=${PYTHON:-python3}
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../../../.." && pwd)
cache=${VAEG_CACHE_DIR:-${HOME}/work/pc88va/pc88va-private-docs/pc88va-utilities-cache}
msdos4=$cache/msdos4-va.2/dl/msdos4-pc88va-2hd.d88
if [ "${1:-}" = "--msdos4" ]; then
	msdos4=$2
	shift 2
fi
if [ "$#" -ne 1 ]; then
	printf 'usage: %s [--msdos4 IMAGE] OUTPUT.d88\n' "$0" >&2
	exit 2
fi
output=$1
[ ! -e "$output" ] || { printf 'error: %s exists\n' "$output" >&2; exit 1; }
[ -f "$msdos4" ] || { printf 'error: MS-DOS 4.0 image not found: %s\n' "$msdos4" >&2; exit 1; }
echo '7c4b141d31e0034120e0b07eb93b9bea808e1c54822e48e15189cc7398385db0  '"$msdos4" |
	sha256sum -c - >/dev/null || { printf 'error: not the msdos4-va.2 release image\n' >&2; exit 1; }

work=$(mktemp -d "${TMPDIR:-/tmp}/vaeg-hfva.XXXXXX")
trap 'rm -rf "$work"' EXIT
root=$work/payload/root
mkdir -p "$root"
build_id=${BUILD_ID:-$(git -C "$script_dir" rev-parse HEAD 2>/dev/null | cut -c1-8 || true)}
[ -n "$build_id" ] || build_id=nogit000
"$nasm_command" -f bin -I"$script_dir/" "-DBUILD_ID='$build_id'" \
	"$script_dir/hfv0.asm" -o "$root/HFV0.COM"
"$nasm_command" -f bin -I"$script_dir/" "-DBUILD_ID='$build_id'" \
	"$script_dir/hfv1.asm" -o "$root/HFV1.COM"
sed 's/$/\r/' "$script_dir/hfv.txt" >"$root/HFV.TXT"
"$python_command" -I "$repo_root/tools/pc88va/pcengine_disk.py" vanilla --source "$msdos4" --output "$output"
"$python_command" -I "$repo_root/tools/pc88va/pcengine_disk.py" install --image "$output" --payload "$work/payload"
printf 'build id %s\n' "$build_id"
sha256sum "$root/HFV0.COM" "$root/HFV1.COM" "$output"
