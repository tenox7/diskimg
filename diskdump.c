// DiskDump 1.0 by Antoni Sawicki <as@tenoware.com>
// Dumps raw sectors of physical drive in to a file
// Allows for an offset / no. 512 sectors to skip
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdarg.h>

#define BUFFER_SIZE 65536 //65k seems to be the standard for "sequential io"

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WDATE__ WIDEN(__DATE__)
#define __WTIME__ WIDEN(__TIME__)

#define USAGE L"Usage: diskdump <disk#> <filename> [sect_skip]\n\n"\
              L"Writes contents of <disk#> info file <filename>\n\n"\
              L"Disk# number can be obtained from:\n"\
              L"- Disk Management (diskmgmt.msc)\n"\
              L"- diskpart (list disk)\n"\
              L"- wmic diskdrive get index,caption,size\n"\
              L"- get-disk\n"\
              L"- get-physicaldisk | ft deviceid,friendlyname\n\n"\
              L"Long form \\\\.\\PhysicalDriveXX is also allowed\n\n"\
              L"sect_skip is number of 512 bytes sectors to skip\n\n"

void error(int exit, WCHAR *msg, ...) {
    va_list valist;
    WCHAR vaBuff[1024]={L'\0'};
    WCHAR errBuff[1024]={L'\0'};
    DWORD err;

    err=GetLastError();

    va_start(valist, msg);
    _vsnwprintf_s(vaBuff, sizeof(vaBuff), sizeof(vaBuff), msg, valist);
    va_end(valist);

    wprintf(L"\n\n%s: %s\n", (exit) ? L"ERROR":L"WARNING", vaBuff);

    if (err) {
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errBuff, sizeof(errBuff), NULL);
        wprintf(L"[0x%08X] %s\n\n", err, errBuff);
    }
    else {
        putchar(L'\n');
    }

    FlushFileBuffers(GetStdHandle(STD_OUTPUT_HANDLE));

    if(exit)
        ExitProcess(1);
}

int wmain(int argc, WCHAR *argv[]) {
    HANDLE                  hDisk;
    HANDLE                  hFile;
    WCHAR                   DevName[64]={'\0'};
    WCHAR                   Buff[BUFFER_SIZE]={'\0'};
    WCHAR                   *DiskNo;
    WCHAR                   *FileName;
    ULONG                   BytesRet;
    GET_LENGTH_INFORMATION  DiskLengthInfo;
    LARGE_INTEGER           Offset;
    LARGE_INTEGER           TotalBytesRead;
    LARGE_INTEGER           FileSize;
    LARGE_INTEGER           Zero;
    size_t                  size=0;
    DWORD                   BytesRead=0;
    DWORD                   ret=0;
    int                     n=0;
    int                     nth=25; // how often to print status
    LARGE_INTEGER           pres, pstart, pend;
    STORAGE_PROPERTY_QUERY  desc_q = { StorageDeviceProperty,  PropertyStandardQuery };
    STORAGE_DESCRIPTOR_HEADER desc_h = { 0 };
    PSTORAGE_DEVICE_DESCRIPTOR desc_d;
    WCHAR *ft[] = { L"Non-Removable", L"Removable" };
    WCHAR *bus[] = { L"UNKNOWN", L"SCSI", L"ATAPI", L"ATA", L"1394", L"SSA", L"FC", L"USB", L"RAID", L"ISCSI", L"SAS", L"SATA", L"SD", L"MMC", L"VIRTUAL", L"VHD", L"MAX", L"NVME"};

    wprintf(L"DiskDump v1.0 by Antoni Sawicki <as@tenoware.com>, Build %s %s\n\n", __WDATE__, __WTIME__);

    if(argc < 3) 
        error(1, L"Wrong number of parameters [argc=%d]\n\n%s\n", argc, USAGE);

    DiskNo = argv[1];
    FileName = argv[2];
    Offset.QuadPart = (argc==4) ? _wtoi64(argv[3]) * 512 : 0;
 
    if(_wcsnicmp(DiskNo, L"\\\\.\\PhysicalDrive", wcslen(L"\\\\.\\PhysicalDrive")) == 0)
        wcsncpy_s(DevName, sizeof(DevName), DiskNo, sizeof(DevName));
    else if(iswdigit(DiskNo[0]))
        _snwprintf_s(DevName, sizeof(DevName) / sizeof(WCHAR), sizeof(DevName), L"\\\\.\\PhysicalDrive%s", DiskNo);
    else
        error(1, USAGE, argv[0]);

    // Open Disk
    if((hDisk = CreateFileW(DevName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
        error(1, L"Cannot open %s", DevName);

    if(DeviceIoControl(hDisk, FSCTL_ALLOW_EXTENDED_DASD_IO, NULL, 0, NULL, 0, &BytesRet, NULL))
        error(1, L"Error on DeviceIoControl FSCTL_ALLOW_EXTENDED_DASD_IO");

    if(!DeviceIoControl(hDisk, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &DiskLengthInfo, sizeof(GET_LENGTH_INFORMATION), &BytesRet, NULL))
        error(1, L"Error on DeviceIoControl IOCTL_DISK_GET_LENGTH_INFO [%d] ", BytesRet);

    if (!DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &desc_q, sizeof(desc_q), &desc_h, sizeof(desc_h), &BytesRet, NULL))
        error(1, L"Error on DeviceIoControl IOCTL_STORAGE_QUERY_PROPERTY Device Property [%d] ", BytesRet);

    desc_d = malloc(desc_h.Size);
    ZeroMemory(desc_d, desc_h.Size);

    if (!DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &desc_q, sizeof(desc_q), desc_d, desc_h.Size, &BytesRet, NULL))
        error(1, L"Error on DeviceIoControl IOCTL_STORAGE_QUERY_PROPERTY [%d] ", BytesRet);

    if(desc_d->Version != sizeof(STORAGE_DEVICE_DESCRIPTOR)) 
        error(1, L"STORAGE_DEVICE_DESCRIPTOR is wrong size [%d] should be [%d]", desc_d->Version, sizeof(STORAGE_DEVICE_DESCRIPTOR));

    wprintf(L"Disk %s: %s %s %S %S Size: %.1f MB  (%llu bytes)  \n", 
            DiskNo,
            (desc_d->RemovableMedia<=1)  ? ft[desc_d->RemovableMedia] : L"(n/a)",
            (desc_d->BusType<=17)        ? bus[desc_d->BusType] : bus[0],
            (desc_d->VendorIdOffset) ? (char*)desc_d+desc_d->VendorIdOffset : "n/a", 
            (desc_d+desc_d->ProductIdOffset) ? (char*)desc_d+desc_d->ProductIdOffset : "n/a",
            (float)DiskLengthInfo.Length.QuadPart / 1024.0 / 1024.0,
            DiskLengthInfo.Length.QuadPart
    );

    if(SetFilePointerEx(hDisk, Offset, NULL, FILE_BEGIN) == 0) 
        error(1, L"Unable to Set File Pointer for Offset [%ull] ", Offset.QuadPart);

    if(Offset.QuadPart > 0)
        DiskLengthInfo.Length.QuadPart -= Offset.QuadPart;

    // Open File
    if((hFile = CreateFileW(FileName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) 
        error(1, L"Unable to open file %s ", FileName);

    TotalBytesRead.QuadPart=0;

    do {
        ZeroMemory(Buff, BUFFER_SIZE);

        if (ReadFile(hDisk, Buff, BUFFER_SIZE, &BytesRead, NULL) == 0) {
            // End of disk, nothing left
            if(DiskLengthInfo.Length.QuadPart-TotalBytesRead.QuadPart == 0 ) {
                break;
            }
            // End of disk, dangling bytes left (< buffer size)
            else if(DiskLengthInfo.Length.QuadPart-TotalBytesRead.QuadPart < BUFFER_SIZE) {
                if (ReadFile(hDisk, Buff, DiskLengthInfo.Length.QuadPart-TotalBytesRead.QuadPart, &BytesRead, NULL) == 0)
                    error(1, L"While reading last sector of the disk, Status=%d BytesToRead=%llu BytesRead=%d", ret, DiskLengthInfo.Length.QuadPart-TotalBytesRead.QuadPart, BytesRead);
            }
            else {
                error(0, L"While reading disk, Status=%d DiskLength=%llu TotalBytesRead=%llu Diff=%llu Offset=%llu BytesRead=%d\n", ret, DiskLengthInfo.Length.QuadPart, TotalBytesRead.QuadPart, DiskLengthInfo.Length.QuadPart-TotalBytesRead.QuadPart, Offset.QuadPart, BytesRead);
            }
        }

        if (WriteFile(hFile, Buff, BytesRead, NULL, NULL) == 0)
            error(1, L"Error writing to file");
        
        TotalBytesRead.QuadPart+=BytesRead;
        
        if(n++ % nth==0)
            wprintf(L"* [%d] [%.1f MB] [%.1f%%]                 \r", 
                BytesRead, 
                (float)TotalBytesRead.QuadPart/1024.0/1024.0, 
                (float)TotalBytesRead.QuadPart*100/DiskLengthInfo.Length.QuadPart
            );
    }
    while (BytesRead!=0);

    FlushFileBuffers(hFile);

    wprintf(L"\rDone! [%.1f MB] (%llu bytes) [%.1f%%]                  \n", (float)TotalBytesRead.QuadPart/1024.0/1024.0, TotalBytesRead.QuadPart, (float)TotalBytesRead.QuadPart*100.0/DiskLengthInfo.Length.QuadPart );

    GetFileSizeEx(hFile, &FileSize);

    if(FileSize.QuadPart != DiskLengthInfo.Length.QuadPart)
        wprintf(L"WARNING: Disk Size is %llu bytes, File Size is %llu bytes, Difference is %llu bytes!\n", DiskLengthInfo.Length.QuadPart, FileSize.QuadPart, DiskLengthInfo.Length.QuadPart-FileSize.QuadPart);

    if(Offset.QuadPart > 0)
        wprintf(L"Skipped %llu sectors / %llu bytes in the begining\n", Offset.QuadPart/512, Offset.QuadPart);

    CloseHandle(hFile);
    CloseHandle(hDisk);

    return 0;
}
