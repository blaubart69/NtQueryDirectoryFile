#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "LibNtEnum/NtEnum.h"
#include "NtQuery.h"

void myApcCallback(SPI_FILE_DIRECTORY_INFORMATION* FileInfoBuffer, int success, long ntStatus, WCHAR* NtApinameError)
{

}

BOOL RunEnumApc(LPCWSTR NtObjDirname, int NtObjDirnameLen)
{
	long ntstat;
	WCHAR *NtApinameError;

	myNtEnumApcStart(
		NtObjDirname
		, NtObjDirnameLen
		, myApcCallback
		, GetProcessHeap()
		, &ntstat
		, &NtApinameError);

}