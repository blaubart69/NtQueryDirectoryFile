#include "stdafx.h"

typedef struct _opts
{
	BOOL sum;
} OPTS;

typedef struct _stats
{
	ULONGLONG files;
	ULONGLONG dirs;
	ULONGLONG fileSize;
} STATS;

STATS g_stats;
OPTS g_opts;

BOOL isDirectory(const DWORD dwFileAttributes)
{
	return (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

//-------------------------------------------------------------------------------------------------
BOOL IsDotDir(LPCWSTR cFileName, const DWORD cFilenameLenBytes, const DWORD dwFileAttributes) {
	//-------------------------------------------------------------------------------------------------

	if ((dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) return FALSE;
	if (cFileName[0] != L'.') return FALSE;
	if (cFilenameLenBytes == 2) return TRUE;
	if (cFileName[1] != L'.') return FALSE;
	if (cFilenameLenBytes == 4) return TRUE;

	return FALSE;
}

void OnEntry(LPCWSTR NtObjDirname, const USHORT byteDirnameLength, const FILE_DIRECTORY_INFORMATION* entry)
{
	DWORD written;
	writeOut(L"%s\\", NtObjDirname);
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), entry->FileName, entry->FileNameLength / sizeof(WCHAR), &written, NULL);
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"\n", 1, &written, NULL);
}

void OnDirectoryBuffer(LPCWSTR NtObjDirname, const USHORT byteDirnameLength, const FILE_DIRECTORY_INFORMATION* dirBuffer, const int success, const long ntStatus, const WCHAR* NtApinameError)
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
				g_stats.dirs += 1;
			}
			else
			{
				g_stats.files += 1;
				g_stats.fileSize += dirBuffer->EndOfFile;
			}

			if (!g_opts.sum)
			{
				OnEntry(NtObjDirname, byteDirnameLength, dirBuffer);
			}
		}

		hasMore = dirBuffer->NextEntryOffset != 0;
		dirBuffer = (FILE_DIRECTORY_INFORMATION*)((char*)dirBuffer + dirBuffer->NextEntryOffset);
	} while (hasMore);
}

BOOL RunEnumApc(LPCWSTR dirname, int NtObjDirnameLen, BOOL sum)
{
	long ntstat;
	WCHAR *NtApinameError;

	myNtEnumApcInit(OnDirectoryBuffer, GetProcessHeap());
	RtlFillMemory(&g_stats, sizeof(STATS), 0);
	RtlFillMemory(&g_opts, sizeof(OPTS), 0);
	g_opts.sum = sum;

	BOOL ok = myNtEnumApcStart(
		dirname
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

	if (sum)
	{
		writeOut(L"dirs/files/filesize\t%I64u/%I64u/%I64u", g_stats.dirs, g_stats.files, g_stats.fileSize);
	}

	return ok;
}