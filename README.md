Tools for imaging hard disks, removable media and floppy under Windows.
Yet another Rawrite or DD for Windows.

* diskdump - saves contents of a disk in to a file; allows sector skip and max bytes if you want to dump specific region and length
* diskrestore - restores contents of an image file to a disk; allows sector skip and nul file for just erasing media (aka `dd if=/dev/zero`)

These utilities are useful for targetting specific targets of [SCSI2SD](https://github.com/vivier/SCSI2SD) media by offset.

