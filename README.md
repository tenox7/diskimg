Tools for imaging hard disks, removable media and floppy under Windows.
Yet another Rawrite or DD for Windows. Mostly used for manipulating 
[SCSI2SD](http://www.codesrc.com/mediawiki/index.php?title=SCSI2SD) disk images.

## diskdump

* saves contents of a disk in to a file
* allows sector skip and max bytes if you want to dump specific region and length

## diskrestore 

* restores contents of an image file (raw/dd) to a disk
* allows sector skip (aka `dd seek=N`) useful for SCSI2SD
* supports nul file for just erasing media (aka `dd if=/dev/zero`)

Sector skip is useful for reading and writing images in specific targets of [SCSI2SD](http://www.codesrc.com/mediawiki/index.php?title=SCSI2SD) media by offset.

## diskclean

* quickly cleans disk layout, partitions, mbr
* exactly same as `diskpart clean` but without waiting for VDS
* does NOT perform full format / data erase

## diskeject

* ejects removable media

## disktrim

[disktrim](https://github.com/tenox7/disktrim) is a separate project

## list disks

- [lsblk](https://github.com/tenox7/lsblk)
- [listdisk](https://github.com/tenox7/listdisk)

## legal

Improper use may cause data loss. I take no resposibility for your data what so ever.

```
Copyright (c) 2006-2018 by Antoni Sawicki
Copyright (c) 2019-2022 by Google LLC
License: Apache 2.0
```
