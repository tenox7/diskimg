#!/bin/bash -e
# disk image restore for macos
usg="usage: $0 [-z] <file> <disk#>"
[ "${1}" = "-z" ] && { erase=true; shift; }
src="${1?:No src file specified, ${usg}}"
dst="/dev/rdisk${2?:No dst disk# specified, ${usg}}"
bs="1m"
diskutil unmountDisk "${dst}"
[ "${erase}" = true ] && diskutil zeroDisk short "${dst}"
case "${src}" in
    *.lz)  cmd="lzip -dc ${src}";;
    *.xz)  md="xz -dc ${src}";;
    *.gz)  cmd="gzip -dc ${src}";;
    *.bz2) cmd="bzip2 -dc ${src}";;
    *)     cmd="dd if=${src} bs=${bs} status=none";;
esac
${cmd} | dd of="${dst}" bs="$bs" status="progress"
diskutil eject "${dst}"
