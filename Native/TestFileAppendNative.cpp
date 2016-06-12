// TestFileAppend.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace std;

DWORD _pid = 42;

PCWSTR fileName = L"c:\\tmp\\log\\test.txt";

void DoTrace(int loops)
{
    // https://blogs.msdn.microsoft.com/oldnewthing/20151127-00/?p=92211/
    // https://msdn.microsoft.com/en-us/library/ff548289.aspx
    // If only the FILE_APPEND_DATA and SYNCHRONIZE flags are set, the caller can write only to the end of the file, 
    // and any offset information about writes to the file is ignored.
    // However, the file will automatically be extended as necessary for this type of write operation.
    HANDLE file = ::CreateFileW(fileName, FILE_APPEND_DATA | SYNCHRONIZE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        DebugBreak();
        return;
    }

    char buf[64 * 1024];
    int chars = sprintf_s(buf, "%.5u XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n", _pid);
    if (chars <= 0)
    {
        DebugBreak();
        return;
    }

    for (int i = 0; i < loops; ++i)
    {
        DWORD written = 0;
        ::WriteFile(file, buf, (DWORD)chars, &written, NULL);
        if ((DWORD)chars != written)
        {
            DebugBreak();
            return;
        }
        //Sleep(1);
    }

    ::CloseHandle(file);
}

void DoVerify()
{
    HANDLE file = ::CreateFileW(fileName, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        printf("DoVerify: can't open file\n");
        DebugBreak();
        return;
    }
    DWORD sizeHigh = 0;
    DWORD sizeLow = ::GetFileSize(file, &sizeHigh);
    
    if (sizeLow == 0)
    {
        printf("DoVerify: file empty\n");
        DebugBreak();
        return;
    }
    UINT64 fileSize = (UINT64)sizeLow + (((UINT64)sizeHigh)<<32);
    HANDLE fileMapping = ::CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
    if (fileMapping == NULL)
    {
        printf("DoVerify: can't create file mapping\n");
        DebugBreak();
        return;
    }

    PCSTR bytes = (PCSTR)::MapViewOfFile(fileMapping, FILE_MAP_READ, 0,0,0);
    if (bytes == NULL)
    {
        printf("DoVerify: can't create file view\n");
        DebugBreak();
        return;
    }

    map<int, int> pids2lines;
    PCSTR pos = bytes;
    PCSTR end = &bytes[fileSize];
    while (pos + 1007 <= end)
    {
        int pid = atoi(pos);
        if (pid == 0)
        {
            printf("DoVerify: Invalid PID at offset %X\n", (unsigned)(pos - bytes));
            DebugBreak();
            return;
        }

        pos += 5;

        if (*pos != ' ')
        {
            printf("DoVerify: no space after PID at offset %X\n", (unsigned)(pos - bytes));
            DebugBreak();
            return;
        }
        ++pos;

        if (0 != memcmp(pos, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n", 1001))
        {
            printf("DoVerify: wrong pattern at offset %X\n", (unsigned)(pos - bytes));
            DebugBreak();
            return;
        }
        pos += 1001;

        auto pidSlot = pids2lines.find(pid);
        if (pidSlot == pids2lines.end())
        {
            pids2lines[pid] = 1;
        }
        else
        {
            pidSlot->second = pidSlot->second + 1;
        }
    }

    printf("There're %d PIDs:\n", (int)pids2lines.size());
    for (auto& pid : pids2lines)
    {
        printf("%.5d : %d\n", pid.first, pid.second);
    }
}

int main(int argc, char* argv[])
{
    _pid = ::GetCurrentProcessId();

    bool trace = true;
    int loops = 100000;

    if (argc == 2)
    {
        if (*argv[1] == 'v')
        {
            trace = false;
        }
    }
    else if (argc == 3 && *argv[1] == 'l')
    {
        loops = atoi(argv[2]);
    }

    if (trace)
        DoTrace(loops);
    else
        DoVerify();

    return 0;
}

