#include <ntifs.h>
#include <wdm.h>

#include "NtEnum.h"

NTSTATUS myNtOpenFile(LPCWSTR NtObjDirname, USHORT byteDirnameLength, PHANDLE dirHandle);

const SIZE_T dirBufSize = 4096;
//
// GLOBALS
//
DECLSPEC_ALIGN(64) HANDLE				g_ProcessHeap;
DECLSPEC_ALIGN(64) pfDirBufferApc		g_userCallback;
DECLSPEC_ALIGN(64) long*				g_ApcFired;
DECLSPEC_ALIGN(64) long*				g_ApcCompleted;
//
// APC context
//
typedef struct _myApcContext
{
	DECLSPEC_ALIGN(64) IO_STATUS_BLOCK					ioBlock;
	DECLSPEC_ALIGN(64) HANDLE							dirHandle;
	DECLSPEC_ALIGN(64) USHORT							byteLenNtObjDirname;
	DECLSPEC_ALIGN(64) WCHAR							NtObjDirname[1];
	DECLSPEC_ALIGN(64) SPI_FILE_DIRECTORY_INFORMATION	dirBuffer[1];
} MyApcContext;
//
//
//
MyApcContext* AllocContextStruct(LPCWSTR dirname, const USHORT byteLenNtObjDirname, HANDLE dirHandle)
{
	MyApcContext* ctx = (MyApcContext*)
		RtlAllocateHeap(
			g_ProcessHeap, 
			0, 
			  dirBufSize 
			+ byteLenNtObjDirname + sizeof(WCHAR)
			+ FIELD_OFFSET(MyApcContext, dirBuffer));

	if (ctx != NULL)
	{
		ctx->ioBlock.Information = 0;
		ctx->dirHandle = dirHandle;
		ctx->byteLenNtObjDirname = byteLenNtObjDirname;
		memcpy(ctx->NtObjDirname, dirname, byteLenNtObjDirname + sizeof(WCHAR)); // copy with terminating zero
	}

	return ctx;
}
void NTAPI NtQueryDirectoryApcCallback(_In_ PVOID ApcContext, _In_ PIO_STATUS_BLOCK IoStatusBlock, _In_ ULONG Reserved)
{
	MyApcContext *ctx = (MyApcContext*)ApcContext;
	BOOLEAN ApcSubmittedOk = FALSE;

	if (!NT_SUCCESS(IoStatusBlock->Status))
	{
		if (IoStatusBlock->Status != STATUS_NO_MORE_FILES)
		{
			g_userCallback(ctx->NtObjDirname, ctx->byteLenNtObjDirname, ctx->dirBuffer, FALSE, IoStatusBlock->Status, L"NtQueryDirectoryFile(APC)");
		}
	}
	else
	{
		if (IoStatusBlock->Information != 0)
		{
			g_userCallback(ctx->NtObjDirname, ctx->byteLenNtObjDirname, ctx->dirBuffer, TRUE, IoStatusBlock->Status, NULL);

			NTSTATUS ntStat = NtQueryDirectoryFile(
				ctx->dirHandle
				, NULL				// event
				, NtQueryDirectoryApcCallback		// ApcRoutine
				, ctx				// ApcContext
				, &(ctx->ioBlock)
				, ctx->dirBuffer
				, dirBufSize
				, FileDirectoryInformation
				, FALSE				// ReturnSingleEntry
				, NULL				// FileName
				, FALSE				// RestartScan
			);
			if (NT_SUCCESS(ntStat))
			{
				*g_ApcFired += 1;
				ApcSubmittedOk = TRUE;
			}
			else
			{
				g_userCallback(ctx->NtObjDirname, ctx->byteLenNtObjDirname, ctx->dirBuffer, FALSE, ntStat, L"NtQueryDirectoryFile(APC)");
			}
		}
	}

	if (!ApcSubmittedOk)
	{
		RtlFreeHeap(g_ProcessHeap, 0, ApcContext);
	}

	*g_ApcCompleted += 1;
}

int myNtEnumApcStart(
	LPCWSTR NtObjDirname
	, USHORT byteDirnameLength
	, pfDirBufferApc dirBufCallback
	, HANDLE hProcessHeap
	, long* ApcFired
	, long* ApcCompleted
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
	g_ProcessHeap = hProcessHeap;
	g_userCallback = dirBufCallback;
	g_ApcFired = ApcFired;
	g_ApcCompleted = ApcCompleted;
	//
	//
	//
	MyApcContext* ctx = AllocContextStruct(NtObjDirname, byteDirnameLength, dirHandle);
	if (ctx == NULL)
	{
		*NtApinameError = L"RtlAllocateHeap";
		*ntstatus = ntStat;
		return FALSE;
	}

	ntStat = NtQueryDirectoryFile(
		dirHandle
		, NULL								// event
		, NtQueryDirectoryApcCallback		// ApcRoutine
		, ctx								// ApcContext
		, &(ctx->ioBlock)
		, ctx->dirBuffer
		, dirBufSize
		, FileDirectoryInformation
		, FALSE								// ReturnSingleEntry
		, NULL								// FileName
		, FALSE								// RestartScan
	);

	if (NT_SUCCESS(ntStat))
	{
		*g_ApcFired += 1;
	}	
	else
	{
		*NtApinameError = L"NtQueryDirectoryFile(firstCall)";
		*ntstatus = ntStat;
		return FALSE;
	}

	return TRUE;
}
