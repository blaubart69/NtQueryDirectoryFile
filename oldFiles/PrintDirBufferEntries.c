#include "stdafx.h"

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

void printFileEntries(FILE_DIRECTORY_INFORMATION *dirBuffer)
{
	BOOL hasMore;
	do
	{
		if (!IsDotDir(dirBuffer->FileName, dirBuffer->FileNameLength, dirBuffer->FileAttributes))
		{
			DWORD written;
			WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), dirBuffer->FileName, dirBuffer->FileNameLength / sizeof(WCHAR), &written, NULL);
			WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"\n", 1, &written, NULL);
		}
		hasMore = dirBuffer->NextEntryOffset != 0;
		dirBuffer = (FILE_DIRECTORY_INFORMATION*)((char*)dirBuffer + dirBuffer->NextEntryOffset);
	} while (hasMore);
}
