#include <ntifs.h>
#include <wdm.h>

#include "Win32.h"

NTSTATUS myNtOpenFile(LPCWSTR NtObjDirname, USHORT byteDirnameLength, PHANDLE dirHandle)
{
	NTSTATUS ntStat;
	IO_STATUS_BLOCK ioBlock;

	UNICODE_STRING dirname;
	dirname.Buffer = NtObjDirname;
	dirname.Length = byteDirnameLength;
	dirname.MaximumLength = byteDirnameLength + 2;

	OBJECT_ATTRIBUTES objAttr;
	InitializeObjectAttributes(&objAttr, &dirname, OBJ_INHERIT, NULL, NULL);

	ntStat = NtOpenFile(
		dirHandle
		, GENERIC_READ
		, (POBJECT_ATTRIBUTES)&objAttr
		, &ioBlock
		, FILE_SHARE_READ
		, FILE_DIRECTORY_FILE);

	return ntStat;
}

int myCreateFile(LPCWSTR lpFilename, PHANDLE dirHandle)
{
	*dirHandle =
		CreateFileW(
			lpFilename
			, GENERIC_READ
			, FILE_SHARE_READ
			, NULL
			, OPEN_EXISTING
			, FILE_FLAG_OVERLAPPED | FILE_FLAG_BACKUP_SEMANTICS
			, NULL);
	
	return *dirHandle != INVALID_HANDLE_VALUE;
}