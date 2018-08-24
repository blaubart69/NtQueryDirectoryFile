// #if !(defined(_X86_) || defined(_AMD64_) || defined(_ARM_) || defined(_ARM64_))
//#define _AMD64_ --> commandline option in project settings!

/*

#include "NtEnum.h"

long myNtEnumSeq(LPCWSTR NtObjDirname, USHORT byteDirnameLength, pfDirBufferCallback dirBufCallback
	, PVOID bufferToUse, unsigned long bufferSize, WCHAR** NtApinameError) 
{
	*NtApinameError = NULL;
	NTSTATUS ntStat;

	HANDLE dirHandle;
	ntStat = myNtOpenFile(NtObjDirname, byteDirnameLength, &dirHandle);
	if (!NT_SUCCESS(ntStat))
	{
		*NtApinameError = L"NtOpenFile";
		return ntStat;
	}

	LARGE_INTEGER timeout = { .QuadPart = -1 };
	IO_STATUS_BLOCK ioBlock;

	for (;;)
	{
		ioBlock.Information = 0;

		ntStat = NtQueryDirectoryFile(
			dirHandle
			, NULL		// event
			, NULL		// ApcRoutine
			, NULL		// ApcContext
			, &ioBlock
			, bufferToUse
			, bufferSize
			, FileDirectoryInformation
			, FALSE		// ReturnSingleEntry
			, NULL		// FileName
			, FALSE		// RestartScan
		);

		ZwWaitForSingleObject(dirHandle, FALSE, &timeout);

		if (NT_SUCCESS(ntStat))
		{
			if (ioBlock.Information == 0)
			{
				ntStat = STATUS_NO_MORE_FILES;
				break;
			}
			dirBufCallback(bufferToUse);
		}
		else
		{
			*NtApinameError = L"NtQueryDirectoryFile";
			break;
		}
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
}

*/