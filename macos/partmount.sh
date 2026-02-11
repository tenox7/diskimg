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
    hdiutil attach -nomount -plist "$img" > "$plist" || die "hdiutil attach failed"

    local count=$(plutil -extract system-entities raw "$plist")
    local basedev="" slicedev=""
    for ((i=0; i<count; i++)); do
        local dev=$(pval ":system-entities:${i}:dev-entry" "$plist")
        local hint=$(pval ":system-entities:${i}:content-hint" "$plist")
        [[ "$hint" == *partition_scheme* ]] && basedev=$dev
        [[ "$dev" == *s${partnum} ]] && slicedev=$dev
    done
    [ -n "$basedev" ] || die "failed to get base device"
    [ -n "$slicedev" ] || die "partition $partnum not found in attached devices"

    if [ -n "$mntpt" ]; then
        diskutil mount -mountPoint "$mntpt" "$slicedev" || { hdiutil detach "$basedev"; die "mount failed"; }
    else
        diskutil mount "$slicedev" || { hdiutil detach "$basedev"; die "mount failed"; }
    fi
    echo "Attached image on $basedev, mounted $slicedev"
}

cmd_umount() {
    [ -n "$1" ] || die "device or mountpoint required"
    local dev=$1
    if [[ "$dev" == *s* ]]; then
        local basedev=${dev%s*}
        diskutil unmount "$dev"
        hdiutil detach "$basedev"
    else
        hdiutil detach "$dev"
    fi
}

[ $# -ge 1 ] || usage

case $1 in
    list)   cmd_list "$2" ;;
    mount)  cmd_mount "$2" "$3" "$4" ;;
    umount) cmd_umount "$2" ;;
    *)      usage ;;
esac
