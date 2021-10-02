// DiskEject 1.2 by Antoni Sawicki <as@tenoware.com>
// Ejects removable media
//
// Copyright (c) 2006-2018 by Antoni Sawicki
// Copyright (c) 2019-2020 by Google LLC
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

#define USAGE L"Usage: diskeject <disk#> \n\n"\
              L"Ejects removable media.\n\n"\
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
    WCHAR*                  DiskNo;
    DWORD                   BytesRet;

    wprintf(L"DiskEject v1.2.1 by Antoni Sawicki <as@tenoware.com>, Build %s %s\n\n", __WDATE__, __WTIME__);

    if (argc < 2)
        error(1, L"Wrong number of parameters [argc=%d]\n\n%s\n", argc, USAGE);

    DiskNo = argv[1];


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

    // Eject
    DeviceIoControl(hDisk, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &BytesRet, NULL);

    CloseHandle(hDisk);

    return 0;
}
