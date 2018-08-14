// #if !(defined(_X86_) || defined(_AMD64_) || defined(_ARM_) || defined(_ARM64_))
//#define _AMD64_ --> commandline option in project settings!

#include <ntifs.h>
#include <wdm.h>

#include "NtEnum.h"

long myNtEnum(LPCWSTR NtObjDirname, USHORT byteDirnameLength, USHORT byteBufLength, DirBufferCallback dirBufCallback) // L"\\??\\c:\\temp"; 
{
	//RtlDosPathNameToNtPathName_U_WithStatus
	NTSTATUS ntStat;
	IO_STATUS_BLOCK ioBlock;
	HANDLE dirHandle;

	{
		UNICODE_STRING dirname;
		dirname.Buffer			= NtObjDirname;
		dirname.Length			= byteDirnameLength;
		dirname.MaximumLength	= byteBufLength;

		OBJECT_ATTRIBUTES objAttr;
		InitializeObjectAttributes(&objAttr, &dirname, OBJ_INHERIT, NULL, NULL);

		
		if ((ntStat = NtOpenFile(
			&dirHandle
			, GENERIC_READ
			, (POBJECT_ATTRIBUTES)&objAttr
			, &ioBlock
			, FILE_SHARE_READ
			, FILE_DIRECTORY_FILE)) != STATUS_SUCCESS)
		{
			//writeOut(L"E: rc: 0x%lX, API: NtOpenFile\n", ntStat);
			return ntStat;
		}
	}
	__declspec(align(64)) FILE_DIRECTORY_INFORMATION	dirBuffer[128];

	while ((ntStat = NtQueryDirectoryFile(
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
	)) == STATUS_SUCCESS)
	{
		//writeOut(L"E: rc: 0x%lX, API: NtQueryDirectoryFile\n", ntStat);
		dirBufCallback(dirBuffer);
	}

	NtClose(dirHandle);

	if (ntStat == STATUS_NO_MORE_FILES)
	{
		return STATUS_SUCCESS;
	}
	else
	{
		return ntStat;
	}

	/*
	{
		ULONG FileInfoBlocksReceived = ioBlock.Information / sizeof(FILE_DIRECTORY_INFORMATION);
		writeOut(L"%ld file info blocks received. bytes received: %lu. sizeof(FILE_DIRECTORY_INFORMATION): %d\n",
			FileInfoBlocksReceived, (ULONG)ioBlock.Information, sizeof(FILE_DIRECTORY_INFORMATION));

		printFileEntries(dirBuffer);
	}
	*/
}

