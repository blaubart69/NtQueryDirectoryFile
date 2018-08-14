// NtQueryDirectoryFile.cpp : Defines the entry point for the console application.
//

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

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
	while (dirBuffer->NextEntryOffset != 0)
	{
		DWORD written;
		WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), dirBuffer->FileName, dirBuffer->FileNameLength / sizeof(WCHAR), &written, NULL);
		writeOut(L"\n");
		dirBuffer = (SPI_FILE_DIRECTORY_INFORMATION*)((char*)dirBuffer + dirBuffer->NextEntryOffset);
	}
}

void rawmain(void)
{
	WCHAR dirname[32] = L"\\??\\c:\\temp";

	long ntStatus = myNtEnum(dirname, lstrlen(dirname) * sizeof(WCHAR), sizeof(dirname), printFileEntries);
	if (ntStatus != 0)
	{
		writeOut(L"E: rc: 0x%lX, myNtEnum", ntStatus);
	}
	ExitProcess(ntStatus == 0 ? 0 : 8);
}

