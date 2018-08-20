#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "LibNtEnum/NtEnum.h"
#include "NtQuery.h"

BOOL isDirectory(const DWORD dwFileAttributes)
{
	return (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

void OnEntry(LPCWSTR NtObjDirname, USHORT byteDirnameLength, SPI_FILE_DIRECTORY_INFORMATION* entry)
{
	DWORD written;
	writeOut(L"%s\\", NtObjDirname);
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), entry->FileName, entry->FileNameLength / sizeof(WCHAR), &written, NULL);
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"\n", 1, &written, NULL);
}

void OnDirectoryBuffer(LPCWSTR NtObjDirname, const USHORT byteDirnameLength, const SPI_FILE_DIRECTORY_INFORMATION* dirBuffer, const int success, const long ntStatus, const WCHAR* NtApinameError)
{
	if (!success)
	{
		WriteNtError(ntStatus, NtApinameError, L"myApcCallback");
		return;
	}
	BOOL hasMore;
	do
	{
		if (!IsDotDir(dirBuffer->FileName, dirBuffer->FileNameLength, dirBuffer->FileAttributes))
		{
			if (isDirectory(dirBuffer->FileAttributes))
			{
				long ntStatus;
				WCHAR *NtApinameError;
				if (!myNtEnumApcStart(
					NtObjDirname
					, byteDirnameLength
					, dirBuffer->FileName
					, dirBuffer->FileNameLength
					, &ntStatus
					, &NtApinameError))
				{
					WriteNtError(ntStatus, NtApinameError, L"OnDirectoryBuffer");
				}
			}
			OnEntry(NtObjDirname, byteDirnameLength, dirBuffer);
		}

		hasMore = dirBuffer->NextEntryOffset != 0;
		dirBuffer = (SPI_FILE_DIRECTORY_INFORMATION*)((char*)dirBuffer + dirBuffer->NextEntryOffset);
	} while (hasMore);
}

BOOL RunEnumApc(LPCWSTR NtObjDirname, int NtObjDirnameLen)
{
	long ntstat;
	WCHAR *NtApinameError;

	myNtEnumApcInit(OnDirectoryBuffer, GetProcessHeap());

	BOOL ok = myNtEnumApcStart(
		NtObjDirname
		, NtObjDirnameLen * sizeof(WCHAR)
		, NULL
		, 0
		, &ntstat
		, &NtApinameError);

	if (!ok)
	{
		WriteNtError(ntstat, NtApinameError, L"myNtEnumApcStart");
		return FALSE;
	}

	while (TRUE)
	{
		DWORD reason = SleepEx(1000, TRUE);

		long fired, completed;
		myNtEnumApcReportCounter(&fired, &completed);

		if (reason == 0) // "The return value is zero if the specified time interval expired."
		{
			writeOut(L"APCs fired/completed\t%ld/%ld\n",
				fired, completed);
		}
		if (fired == completed)
		{
			break;
		}
	}

	return ok;
}