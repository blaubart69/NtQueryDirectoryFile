// NtQueryDirectoryFile.cpp : Defines the entry point for the console application.
//

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "MyNT.h"

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

void printFileEntries(FILE_DIRECTORY_INFORMATION *dirBuffer)
{
	while (dirBuffer->NextEntryOffset != 0)
	{
		DWORD written;
		WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), dirBuffer->FileName, dirBuffer->FileNameLength / sizeof(WCHAR), &written, NULL);
		writeOut(L"\n");
		dirBuffer = (char*)dirBuffer + dirBuffer->NextEntryOffset;
	}

}

void rawmain(void)
{
	NTSTATUS ntStat;

	__declspec(align(64)) FILE_DIRECTORY_INFORMATION	dirBuffer[128];

	HANDLE dirHandle;

	WCHAR dirnameBuffer[32] = L"\\??\\c:\\temp";
	UNICODE_STRING dirname;
	dirname.Buffer = dirnameBuffer;
	dirname.Length = lstrlen(dirnameBuffer) * sizeof(WCHAR);
	dirname.MaximumLength = sizeof(dirnameBuffer);

	OBJECT_ATTRIBUTES objAttr;
	InitializeObjectAttributes(&objAttr, &dirname, OBJ_INHERIT, NULL, NULL);

	IO_STATUS_BLOCK ioBlock;
	if ( (ntStat = NtOpenFile(
		&dirHandle
		, GENERIC_READ
		, (POBJECT_ATTRIBUTES)&objAttr
		, &ioBlock
		, FILE_SHARE_READ
		, FILE_DIRECTORY_FILE )) != STATUS_SUCCESS )
	{
		writeOut(L"E: rc: 0x%lX, API: NtOpenFile\n", ntStat);
	}
	else if ((ntStat = NtQueryDirectoryFile(
		dirHandle
		, NULL		// event
		, NULL		// ApcRoutine
		, NULL		// ApcContext
		, &ioBlock
		, dirBuffer
		, sizeof(dirBuffer)
		, FileDirectoryInformation
		, FALSE		// ReturnSingleEntry
		, NULL		// FileName
		, FALSE		// RestartScan
		)) != STATUS_SUCCESS)
	{
		writeOut(L"E: rc: 0x%lX, API: NtQueryDirectoryFile\n", ntStat);
	}
	else
	{
		ULONG FileInfoBlocksReceived = ioBlock.Information / sizeof(FILE_DIRECTORY_INFORMATION);
		writeOut(L"%ld file info blocks received. bytes received: %lu. sizeof(FILE_DIRECTORY_INFORMATION): %d\n", 
			FileInfoBlocksReceived, (ULONG)ioBlock.Information, sizeof(FILE_DIRECTORY_INFORMATION));

		printFileEntries(dirBuffer);
	}

	ExitProcess(ntStat);
}

