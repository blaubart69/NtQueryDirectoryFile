#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "LibNtEnum/NtEnum.h"
#include "NtQuery.h"

BOOL RunEnumSeq(LPCWSTR NtObjDirname, int NtObjDirnameLen)
{
	LPWSTR NtApinameError = (LPWSTR)L"n/a";

	long bufferSize = 8192;
	LPVOID dirBuffer = HeapAlloc(GetProcessHeap(), 0, bufferSize);

	long ntStatus = myNtEnum(
		NtObjDirname
		, NtObjDirnameLen * sizeof(WCHAR)
		, printFileEntries
		, dirBuffer
		, bufferSize
		, &NtApinameError);

	HeapFree(GetProcessHeap(), 0, dirBuffer);

	BOOL rc;
	if (ntStatus != 0)
	{
		writeOut(L"E: NTSTATUS: 0x%lX, function: RunEnumSeq, NTApiname: %s", ntStatus, NtApinameError);
		rc = FALSE;
	}
	else
	{
		rc = TRUE;
	}
	return rc;
}