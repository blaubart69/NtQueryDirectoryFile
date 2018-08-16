#include <ntifs.h>
#include <wdm.h>

#include "NtEnum.h"

/*
#define WINAPI      __stdcall
typedef unsigned long       DWORD;

void*
WINAPI
HeapAlloc(
	_In_ HANDLE hHeap,
	_In_ DWORD dwFlags,
	_In_ SIZE_T dwBytes
);
*/

NTSTATUS myNtOpenFile(LPCWSTR NtObjDirname, USHORT byteDirnameLength, PHANDLE dirHandle);

const SIZE_T dirBufSize = 4096;

typedef struct _myApcContext
{
	DECLSPEC_ALIGN(64) IO_STATUS_BLOCK					ioBlock;
	DECLSPEC_ALIGN(64) HANDLE							dirHandle;
	DECLSPEC_ALIGN(64) HANDLE							ProcessHeap;
	DECLSPEC_ALIGN(64) pfDirBufferApc					userCallback;
	DECLSPEC_ALIGN(64) SPI_FILE_DIRECTORY_INFORMATION	dirBuffer[1];
} MyApcContext;

void NTAPI myApcRoutine(_In_ PVOID ApcContext, _In_ PIO_STATUS_BLOCK IoStatusBlock, _In_ ULONG Reserved)
{
	MyApcContext *ctx = (MyApcContext*)ApcContext;

	if (NT_SUCCESS(IoStatusBlock->Status))
	{
		if (IoStatusBlock->Information == 0)
		{
			//ntStat = STATUS_NO_MORE_FILES;
			RtlFreeHeap(ctx->ProcessHeap, 0, ApcContext);
		}
		else
		{
			ctx->userCallback(ctx->dirBuffer, TRUE, IoStatusBlock->Status, NULL);

			NTSTATUS ntStat = NtQueryDirectoryFile(
				ctx->dirHandle
				, NULL				// event
				, myApcRoutine		// ApcRoutine
				, ctx				// ApcContext
				, &(ctx->ioBlock)
				, ctx->dirBuffer
				, dirBufSize
				, FileDirectoryInformation
				, FALSE				// ReturnSingleEntry
				, NULL				// FileName
				, FALSE				// RestartScan
			);
			if (!NT_SUCCESS(ntStat))
			{
				ctx->userCallback(ctx->dirBuffer, FALSE, ntStat, L"NtQueryDirectoryFile(APC)");
				RtlFreeHeap(ctx->ProcessHeap, 0, ApcContext);
			}
		}
	}
	else
	{
		ctx->userCallback(ctx->dirBuffer, FALSE, IoStatusBlock->Status, L"NtQueryDirectoryFile(APC)");
		RtlFreeHeap(ctx->ProcessHeap, 0, ApcContext);
	}
}

int 
myNtEnumApcStart(LPCWSTR NtObjDirname, USHORT byteDirnameLength
	, pfDirBufferApc dirBufCallback
	, HANDLE hProcessHeap
	, long* ntstatus
	, WCHAR** NtApinameError)
{
	NTSTATUS ntStat;

	HANDLE dirHandle;
	ntStat = myNtOpenFile(NtObjDirname, byteDirnameLength, &dirHandle);
	if (!NT_SUCCESS(ntStat))
	{
		*NtApinameError = L"NtOpenFile";
		*ntstatus = ntStat;
		return FALSE;
	}
	//
	//
	//
	MyApcContext* ctx = RtlAllocateHeap(hProcessHeap, 0, dirBufSize + FIELD_OFFSET(MyApcContext, dirBuffer));

	if (ctx == NULL)
	{
		*NtApinameError = L"RtlAllocateHeap";
		*ntstatus = ntStat;
		return FALSE;
	}
	//
	//
	//

	ctx->ioBlock.Information = 0;
	ctx->userCallback = dirBufCallback;
	ctx->dirHandle = dirHandle;

	ntStat = NtQueryDirectoryFile(
		dirHandle
		, NULL				// event
		, myApcRoutine		// ApcRoutine
		, ctx				// ApcContext
		, &(ctx->ioBlock)
		, ctx->dirBuffer
		, dirBufSize
		, FileDirectoryInformation
		, FALSE				// ReturnSingleEntry
		, NULL				// FileName
		, FALSE				// RestartScan
	);

	if (!NT_SUCCESS(ntStat))
	{
		*NtApinameError = L"NtQueryDirectoryFile";
		*ntstatus = ntStat;
		return FALSE;
	}

	return TRUE;
}