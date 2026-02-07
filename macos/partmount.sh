#!/bin/bash

die() { echo "error: $*" >&2; exit 1; }

usage() {
    echo "Usage: $(basename "$0") list <image>"
    echo "       $(basename "$0") mount <image> <part#> [mountpoint]"
    echo "       $(basename "$0") umount <device|mountpoint>"
    exit 1
}

PB=/usr/libexec/PlistBuddy

pval() { $PB -c "Print $1" "$2" 2>/dev/null; }

cmd_list() {
    [ -f "$1" ] || die "file not found: $1"
    local plist=$(mktemp) || die "mktemp failed"
    trap "rm -f $plist" RETURN
    hdiutil imageinfo -plist "$1" > "$plist" || die "hdiutil failed"

    local bs=$(pval :partitions:block-size "$plist")
    local count=$(plutil -extract partitions.partitions raw "$plist")

    printf "%-4s %-12s %-12s %-8s %s\n" "#" "START" "LENGTH" "SIZE" "TYPE"
    for ((i=0; i<count; i++)); do
        local p=":partitions:partitions:${i}"
        [[ $(pval "${p}:partition-synthesized" "$plist") == "true" ]] && continue

        local num=$(pval "${p}:partition-number" "$plist")
        [ -z "$num" ] && continue

        local start=$(pval "${p}:partition-start" "$plist")
        local len=$(pval "${p}:partition-length" "$plist")
        local hint=$(pval "${p}:partition-hint" "$plist")
        local sz=$((len * bs))
        local hr
        if [ "$sz" -ge 1073741824 ]; then hr="$((sz / 1073741824))G"
        elif [ "$sz" -ge 1048576 ]; then hr="$((sz / 1048576))M"
        else hr="$((sz / 1024))K"; fi

        printf "%-4s %-12s %-12s %-8s %s\n" "$num" "$start" "$len" "$hr" "$hint"
    done
}

cmd_mount() {
    [ -f "$1" ] || die "file not found: $1"
    [ -n "$2" ] || die "partition number required"
    local img=$1 partnum=$2 mntpt=$3
    local plist=$(mktemp) || die "mktemp failed"
    trap "rm -f $plist" RETURN
    hdiutil imageinfo -plist "$img" > "$plist" || die "hdiutil failed"

    local bs=$(pval :partitions:block-size "$plist")
    local count=$(plutil -extract partitions.partitions raw "$plist")
    local start="" len=""

    for ((i=0; i<count; i++)); do
        local p=":partitions:partitions:${i}"
        local num=$(pval "${p}:partition-number" "$plist")
        [ "$num" = "$partnum" ] || continue
        start=$(pval "${p}:partition-start" "$plist")
        len=$(pval "${p}:partition-length" "$plist")
        break
    done

    [ -n "$start" ] || die "partition $partnum not found"

    local args=(-imagekey diskimage-class=CRawDiskImage)
    args+=(-imagekey "offset=$((start * bs))")
    args+=(-imagekey "length=$((len * bs))")
    [ -n "$mntpt" ] && args+=(-mountpoint "$mntpt")
    hdiutil attach "${args[@]}" "$img"
}

cmd_umount() {
    [ -n "$1" ] || die "device or mountpoint required"
    hdiutil detach "$1"
}

[ $# -ge 1 ] || usage

case $1 in
    list)   cmd_list "$2" ;;
    mount)  cmd_mount "$2" "$3" "$4" ;;
    umount) cmd_umount "$2" ;;
    *)      usage ;;
esac
