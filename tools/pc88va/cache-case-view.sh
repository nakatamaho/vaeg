# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# Sourced by the media builders. A shared download cache copied through a
# case-insensitive file system can hold ISHARC.COM as isharc.com; the
# builders ask for exact names. vaeg_cache_case_view DIR prints a private
# directory (below TMPDIR) of symbolic links to DIR's entries under their own
# names and, where free, their upper- and lower-case names. Remove it with
# rm -rf, which does not follow the links.
vaeg_cache_case_view() {
	local dir=$1
	local view
	local entry
	local name
	local variant

	view=$(mktemp -d "${TMPDIR:-/tmp}/vaeg-cache-view.XXXXXX") || return 1
	for entry in "$dir"/* "$dir"/.[!.]*; do
		[[ -e $entry || -L $entry ]] || continue
		ln -s -- "$entry" "$view/${entry##*/}"
	done
	for entry in "$dir"/*; do
		[[ -f $entry ]] || continue
		name=${entry##*/}
		for variant in "${name^^}" "${name,,}"; do
			[[ -e $view/$variant || -L $view/$variant ]] ||
				ln -s -- "$entry" "$view/$variant"
		done
	done
	printf '%s\n' "$view"
}

# vaeg_cache_case_view_close VIEW DIR: keep the packages downloaded into
# VIEW (regular files, not partial downloads) in the cache DIR, then remove
# VIEW. Builders call it from their exit trap.
vaeg_cache_case_view_close() {
	local view=$1
	local dir=$2
	local entry
	local name

	[[ -n $view && $view == "${TMPDIR:-/tmp}"/vaeg-cache-view.* && -d $view ]] || return 0
	for entry in "$view"/*; do
		[[ -f $entry && ! -L $entry ]] || continue
		name=${entry##*/}
		[[ $name == *.part.* || -e $dir/$name ]] && continue
		mv -- "$entry" "$dir/$name" || true
	done
	rm -rf -- "$view"
}
