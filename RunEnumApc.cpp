#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "LibNtEnum/NtEnum.h"
#include "NtQuery.h"

typedef struct _stats
{
	long apcFired;
	long apcCompleted;
} Stats;

Stats gStats;

void myApcCallback(LPCWSTR NtObjDirname, USHORT byteDirnameLength, SPI_FILE_DIRECTORY_INFORMATION* dirBuffer, int success, long ntStatus, WCHAR* NtApinameError)
{
	if (!success)
	{
		WriteNtError(ntStatus, NtApinameError, L"myApcCallback");
		return;
	}
	{
		BOOL hasMore;
		do
		{
			if (!IsDotDir(dirBuffer->FileName, dirBuffer->FileNameLength, dirBuffer->FileAttributes))
			{
				DWORD written;
				writeOut(L"%s\\", NtObjDirname);
				WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), dirBuffer->FileName, dirBuffer->FileNameLength / sizeof(WCHAR), &written, NULL);
				WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"\n", 1, &written, NULL);
			}
			hasMore = dirBuffer->NextEntryOffset != 0;
			dirBuffer = (SPI_FILE_DIRECTORY_INFORMATION*)((char*)dirBuffer + dirBuffer->NextEntryOffset);
		} while (hasMore);
	}

}

BOOL RunEnumApc(LPCWSTR NtObjDirname, int NtObjDirnameLen)
{
	long ntstat;
	WCHAR *NtApinameError;

	gStats = { 0 };

	BOOL ok = myNtEnumApcStart(
		NtObjDirname
		, NtObjDirnameLen * sizeof(WCHAR)
		, myApcCallback
		, GetProcessHeap()
		, &gStats.apcFired
		, &gStats.apcCompleted
		, &ntstat
		, &NtApinameError);

	if (!ok)
	{
		WriteNtError(ntstat, NtApinameError, L"myNtEnumApcStart");
	}

	while (true)
	{
		DWORD reason = SleepEx(1000, TRUE);

		if (reason == 0) // "The return value is zero if the specified time interval expired."
		{
			writeOut(L"APCs fired/completed\t%ld/%ld\n",
				gStats.apcFired,
				gStats.apcCompleted);
		}
		else 
		{
			if (gStats.apcFired == gStats.apcCompleted)
			{
				break;
			}
		}
	}

	return ok;
}