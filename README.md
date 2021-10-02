Tools for imaging hard disks, removable media and floppy under Windows.
Yet another Rawrite or DD for Windows.

## diskdump
* saves contents of a disk in to a file
* allows sector skip and max bytes if you want to dump specific region and length

## diskrestore 
* restores contents of an image file to a disk
* allows sector skip 
* supports nul file for just erasing media (aka `dd if=/dev/zero`)

Sector skip is useful for reading and writing images in specific targets of [SCSI2SD](https://github.com/vivier/SCSI2SD) media by offset.

## diskclean
* quickly cleans disk layout, partitions, mbr
* exactly same as `diskpart clean` but without waiting for VDS
* does NOT perform full format / data erase

## diskeject
* ejects removable media

## legal

```
Copyright (c) 2006-2018 by Antoni Sawicki
Copyright (c) 2019 by Google LLC
License: Apache 2.0
```
