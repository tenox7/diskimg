// DiskClean 1.2 by Antoni Sawicki <as@tenoware.com>
// Removes disk layout, partitions, mbr
// Similar to diskpart clean
//
// Copyright (c) 2006-2018 by Antoni Sawicki
// Copyright (c) 2019 by Google LLC
// License: Apache 2.0
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdarg.h>

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WDATE__ WIDEN(__DATE__)
#define __WTIME__ WIDEN(__TIME__)

#define USAGE L"Usage: diskclean <disk#> \n\n"\
              L"Removes disk layout, partitions, mbr.\n"\
              L"Similar to diskpart clean.\n\n"\
              L"Disk# number can be obtained from:\n"\
              L"- Disk Management (diskmgmt.msc)\n"\
              L"- cmd: diskpart> list disk\n"\
              L"- cmd: wmic diskdrive get index,caption,size\n"\
              L"- cmd: lsblk\n"\
              L"- ps: get-disk\n"\
              L"- ps: get-physicaldisk | ft deviceid,friendlyname\n"\
              L"Long form \\\\.\\PhysicalDriveXX is also allowed\n"\

void error(int exit, WCHAR* msg, ...) {
    va_list valist;
    WCHAR vaBuff[1024] = { L'\0' };
    WCHAR errBuff[1024] = { L'\0' };
    DWORD err;

    err = GetLastError();

    va_start(valist, msg);
    vswprintf(vaBuff, ARRAYSIZE(vaBuff), msg, valist);
    va_end(valist);

    wprintf(L"\n\n%s: %s\n", (exit) ? L"ERROR" : L"WARNING", vaBuff);

    if (err) {
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errBuff, ARRAYSIZE(errBuff), NULL);
        wprintf(L"[0x%08X] %s\n\n", err, errBuff);
    }
    else {
        putchar(L'\n');
    }

    FlushFileBuffers(GetStdHandle(STD_OUTPUT_HANDLE));

    if (exit)
        ExitProcess(1);
}

int wmain(int argc, WCHAR* argv[]) {
    HANDLE                  hDisk;
    WCHAR                   DevName[64] = { '\0' };
    WCHAR* DiskNo;
    WCHAR* FileName;
    ULONG                   BytesRet;
    GET_LENGTH_INFORMATION  DiskLengthInfo;
    DISK_GEOMETRY           DiskGeom;
    LARGE_INTEGER           Offset;
    LARGE_INTEGER           TotalBytesRead;
    LARGE_INTEGER           TotalBytesWritten;
    LARGE_INTEGER           FileSize;
    LARGE_INTEGER           Zero;
    size_t                  size = 0;
    DWORD                   ret = 0;
    int                     n = 0;
    LARGE_INTEGER           pres, pstart, pend;
    STORAGE_PROPERTY_QUERY  desc_q = { StorageDeviceProperty,  PropertyStandardQuery };
    STORAGE_DESCRIPTOR_HEADER desc_h = { 0 };
    PSTORAGE_DEVICE_DESCRIPTOR desc_d;
    WCHAR* ft[] = { L"Non-Removable", L"Removable" };
    WCHAR* bus[] = { L"UNKNOWN", L"SCSI", L"ATAPI", L"ATA", L"1394", L"SSA", L"FC", L"USB", L"RAID", L"ISCSI", L"SAS", L"SATA", L"SD", L"MMC", L"VIRTUAL", L"VHD", L"MAX", L"NVME" };

    wprintf(L"DiskClean v1.2.2 by Antoni Sawicki <as@tenoware.com>, Build %s %s\n\n", __WDATE__, __WTIME__);

    if (argc < 2)
        error(1, L"Wrong number of parameters [argc=%d]\n\n%s\n", argc, USAGE);

    DiskNo = argv[1];
    Offset.QuadPart = (argc == 4) ? _wtoi64(argv[3]) * 512 : 0;

    if (wcsncmp(DiskNo, L"\\\\.\\PhysicalDrive", 13) == 0)
        wcsncpy(DevName, DiskNo, ARRAYSIZE(DevName));
    else if (iswdigit(DiskNo[0]))
        swprintf(DevName, ARRAYSIZE(DevName), L"\\\\.\\PhysicalDrive%s", DiskNo);
    else if (DiskNo[0] == 'a' || DiskNo[0] == 'A' || DiskNo[0] == 'b' || DiskNo[0] == 'B')
        swprintf(DevName, ARRAYSIZE(DevName), L"\\\\.\\%c:", DiskNo[0]);
    else
        error(1, USAGE, argv[0]);

    // Open Disk
    if ((hDisk = CreateFileW(DevName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
        error(1, L"Cannot open %s", DevName);

    __try {
        if (iswdigit(DiskNo[0]) && DeviceIoControl(hDisk, FSCTL_ALLOW_EXTENDED_DASD_IO, NULL, 0, NULL, 0, &BytesRet, NULL))
            error(0, L"Error on DeviceIoControl FSCTL_ALLOW_EXTENDED_DASD_IO");

        if (!DeviceIoControl(hDisk, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &BytesRet, NULL))
            error(1, L"Error on DeviceIoControl FSCTL_LOCK_VOLUME [%d] ", BytesRet);

        // Try to obtain disk length. On removable media the first DISK_GET_LENGTH is not supported
        if (!DeviceIoControl(hDisk, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &DiskLengthInfo, sizeof(GET_LENGTH_INFORMATION), &BytesRet, NULL)) {
            if (!DeviceIoControl(hDisk, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &DiskGeom, sizeof(DISK_GEOMETRY), &BytesRet, NULL))
                error(1, L"Error on DeviceIoControl IOCTL_DISK_GET_DRIVE_GEOMETRY [%d] ", BytesRet);

            DiskLengthInfo.Length.QuadPart = DiskGeom.Cylinders.QuadPart * DiskGeom.TracksPerCylinder * DiskGeom.SectorsPerTrack * DiskGeom.BytesPerSector;
        }

        if (!DiskLengthInfo.Length.QuadPart)
            error(1, L"Unable to obtain disk length info");

        if (!DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &desc_q, sizeof(desc_q), &desc_h, sizeof(desc_h), &BytesRet, NULL))
            error(0, L"Error on DeviceIoControl IOCTL_STORAGE_QUERY_PROPERTY Device Property [%d] ", BytesRet);

        desc_d = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, desc_h.Size);

        if (!DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &desc_q, sizeof(desc_q), desc_d, desc_h.Size, &BytesRet, NULL))
            error(0, L"Error on DeviceIoControl IOCTL_STORAGE_QUERY_PROPERTY [%d] ", BytesRet);

        if (desc_d->Version != sizeof(STORAGE_DEVICE_DESCRIPTOR))
            error(0, L"STORAGE_DEVICE_DESCRIPTOR is wrong size [%d] should be [%d]", desc_d->Version, sizeof(STORAGE_DEVICE_DESCRIPTOR));

        wprintf(L"Disk %s %s %s %S %S %.1f MB  (%llu bytes) (0x%llX)  \n",
            DiskNo,
            (desc_d->RemovableMedia <= 1) ? ft[desc_d->RemovableMedia] : L"(n/a)",
            (desc_d->BusType < ARRAYSIZE(bus)) ? bus[desc_d->BusType] : bus[0],
            (desc_d->VendorIdOffset) ? (char*)desc_d + desc_d->VendorIdOffset : "n/a",
            (desc_d + desc_d->ProductIdOffset) ? (char*)desc_d + desc_d->ProductIdOffset : "n/a",
            (float)DiskLengthInfo.Length.QuadPart / 1024.0 / 1024.0,
            DiskLengthInfo.Length.QuadPart,
            DiskLengthInfo.Length.QuadPart
        );
    }
    __except (1) {
    };

    wprintf(L"\nWARNING: you are about to clean your disk erasing all data!\nThere is no going back after this, continue? (y/N) ?");
    if (getwchar() != L'y')
        error(1, L"\rAborting...\n");


    if (!DeviceIoControl(hDisk, IOCTL_DISK_DELETE_DRIVE_LAYOUT, NULL, 0, NULL, 0, &BytesRet, NULL))
        error(1, L"Error on DeviceIoControl IOCTL_DISK_DELETE_DRIVE_LAYOUT [%d] ", BytesRet);

    FlushFileBuffers(hDisk);
    CloseHandle(hDisk);

    return 0;
}
