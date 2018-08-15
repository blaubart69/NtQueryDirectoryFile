// #if !(defined(_X86_) || defined(_AMD64_) || defined(_ARM_) || defined(_ARM64_))
//#define _AMD64_ --> commandline option in project settings!

#include <ntifs.h>
#include <wdm.h>

#include "NtEnum.h"

#define WINAPI				__stdcall
typedef unsigned long       DWORD;
#define DECLSPEC_ALLOCATOR __declspec(allocator)
typedef void               *LPVOID;
typedef int                 BOOL;

//DECLSPEC_ALLOCATOR
LPVOID
WINAPI
HeapAlloc(
	_In_ HANDLE hHeap,
	_In_ DWORD dwFlags,
	_In_ SIZE_T dwBytes
);

BOOL
WINAPI
HeapFree(
	_Inout_ HANDLE hHeap,
	_In_ DWORD dwFlags,
	__drv_freesMem(Mem) _Frees_ptr_opt_ LPVOID lpMem
);

HANDLE
WINAPI
GetProcessHeap(
	VOID
);

long myNtEnum(LPCWSTR NtObjDirname, USHORT byteDirnameLength, USHORT byteBufLength, DirBufferCallback dirBufCallback
	, PVOID bufferToUse, unsigned long bufferSize) 
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

	while ((ntStat = NtQueryDirectoryFile(
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
	)) == STATUS_SUCCESS)
	{
		//writeOut(L"E: rc: 0x%lX, API: NtQueryDirectoryFile\n", ntStat);
		dirBufCallback(bufferToUse);
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

