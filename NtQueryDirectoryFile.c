// NtQueryDirectoryFile.cpp : Defines the entry point for the console application.
//

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#include "LibNtEnum/NtEnum.h"

//#include <ntdef.h>
//#include <ntifs.h>

BOOL writeOut(const WCHAR * format, ...)
{
	va_list args;
	va_start(args, format);
	
	WCHAR buf[1024];
	const int writtenChars = wvsprintfW(buf, format, args);
	va_end(args);

	DWORD written;
	return WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), buf, writtenChars, &written, NULL);
}

void printFileEntries(SPI_FILE_DIRECTORY_INFORMATION *dirBuffer)
{
	BOOL hasMore;
	do
	{
		DWORD written;
		WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), dirBuffer->FileName, dirBuffer->FileNameLength / sizeof(WCHAR), &written, NULL);
		writeOut(L"\n");

		hasMore = dirBuffer->NextEntryOffset != 0;
		dirBuffer = (SPI_FILE_DIRECTORY_INFORMATION*)((char*)dirBuffer + dirBuffer->NextEntryOffset);
	} while (hasMore);
	writeOut(L"---\n");
}

void rawmain(void)
{
	int argc;
	LPWSTR *args = CommandLineToArgvW(GetCommandLineW(), &argc);
	UINT rc;

	if (argc != 2)
	{
		writeOut(L"usage: NtFind {dir}");
		rc = 1;
	}
	else
	{
		WCHAR dirname[1024];
		wsprintfW(dirname, L"\\??\\%s", args[1]);

		long bufferSize = 8192;
		LPVOID dirBuffer = HeapAlloc(GetProcessHeap(), 0, bufferSize);
		long ntStatus = myNtEnum(dirname, lstrlen(dirname) * sizeof(WCHAR), sizeof(dirname), printFileEntries, dirBuffer, bufferSize);
		HeapFree(GetProcessHeap(), 0, dirBuffer);
		if (ntStatus != 0)
		{
			writeOut(L"E: NTSTATUS: 0x%lX, myNtEnum", ntStatus);
			rc = 8;
		}
		else
		{
			rc = 0;
		}
	}
	ExitProcess(rc);
}

