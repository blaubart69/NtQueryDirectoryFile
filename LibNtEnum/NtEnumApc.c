#include <ntifs.h>
#include <wdm.h>

#include "NtEnum.h"

NTSTATUS myNtOpenFile(LPCWSTR NtObjDirname, USHORT byteDirnameLength, PHANDLE dirHandle);

typedef struct _ApcCounter
{
	DECLSPEC_ALIGN(64) long Fired;
	DECLSPEC_ALIGN(64) long Completed;
} ApcCounter;

typedef struct _EnumApcOptions
{
	pfDirBufferApc dirBufCallback;
	HANDLE hProcessHeap;
} EnumApcOptions;

ApcCounter		g_ApcCounter;
EnumApcOptions	g_opts;

const SIZE_T dirBufSize = 4096;

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
MyApcContext* AllocContextStruct(LPCWSTR dirname, const USHORT byteLenNtObjDirname, WCHAR* dirToAppendWithoutZero, USHORT byteLenDirToAppendWithoutZero)
{
	int byteLenFullDir =
		byteLenNtObjDirname
		+ sizeof(WCHAR)			// L"\"
		+ byteLenDirToAppendWithoutZero
		+ sizeof(WCHAR)			// L"\0"
		;

	MyApcContext* ctx = (MyApcContext*)
		RtlAllocateHeap(
			g_opts.hProcessHeap, 
			0, 
			  dirBufSize 
			+ byteLenFullDir
			+ FIELD_OFFSET(MyApcContext, NtObjDirname));

	if (ctx != NULL)
	{
		ctx->ioBlock.Information = 0;
		ctx->byteLenNtObjDirname = byteLenNtObjDirname;

		WCHAR* w = ctx->NtObjDirname;
		memcpy(w, dirname, byteLenNtObjDirname + sizeof(WCHAR)); // copy with terminating zero

		if (byteLenDirToAppendWithoutZero > 0)
		{
			w += byteLenNtObjDirname;
			if (*(w-1) != L'\\')
			{
				*w++ = L'\\';
				ctx->byteLenNtObjDirname += sizeof(WCHAR);
			}
			memcpy(w, dirToAppendWithoutZero, byteLenDirToAppendWithoutZero);
			w += byteLenDirToAppendWithoutZero;
			*w = L'\0';
			ctx->byteLenNtObjDirname += byteLenDirToAppendWithoutZero;
		}
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
			g_opts.dirBufCallback(ctx->NtObjDirname, ctx->byteLenNtObjDirname, ctx->dirBuffer, FALSE, IoStatusBlock->Status, L"NtQueryDirectoryFile(APC)");
		}
	}
	else
	{
		if (IoStatusBlock->Information != 0)
		{
			g_opts.dirBufCallback(ctx->NtObjDirname, ctx->byteLenNtObjDirname, ctx->dirBuffer, TRUE, IoStatusBlock->Status, NULL);

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
				g_ApcCounter.Fired += 1;
				ApcSubmittedOk = TRUE;
			}
			else
			{
				g_opts.dirBufCallback(ctx->NtObjDirname, ctx->byteLenNtObjDirname, ctx->dirBuffer, FALSE, ntStat, L"NtQueryDirectoryFile(APC)");
			}
		}
	}

	if (!ApcSubmittedOk)
	{
		RtlFreeHeap(g_opts.hProcessHeap, 0, ApcContext);
	}

	g_ApcCounter.Completed += 1;
}

int myNtEnumApcStart(
	  LPCWSTR NtObjDirname
	, USHORT byteDirnameLength
	, WCHAR* dirToAppendWithoutZero
	, USHORT byteLenDirToAppendWithoutZero
	, long* ntstatus
	, WCHAR** NtApinameError)
{
	NTSTATUS ntStat;

	MyApcContext* ctx = AllocContextStruct(NtObjDirname, byteDirnameLength, dirToAppendWithoutZero, byteLenDirToAppendWithoutZero);
	if (ctx == NULL)
	{
		*NtApinameError = L"RtlAllocateHeap";
		*ntstatus = -1;
		return FALSE;
	}
	
	HANDLE dirHandle;
	ntStat = myNtOpenFile(ctx->NtObjDirname, ctx->byteLenNtObjDirname, &dirHandle);
	if (!NT_SUCCESS(ntStat))
	{
		*NtApinameError = L"NtOpenFile";
		*ntstatus = ntStat;
		return FALSE;
	}
	ctx->dirHandle = dirHandle;

	ntStat = NtQueryDirectoryFile(
		ctx->dirHandle
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
		g_ApcCounter.Fired += 1;
	}	
	else
	{
		RtlFreeHeap(g_opts.hProcessHeap, 0, ctx);
		*NtApinameError = L"NtQueryDirectoryFile(firstCall)";
		*ntstatus = ntStat;
		return FALSE;
	}

	return TRUE;
}

void myNtEnumApcInit(pfDirBufferApc userCallback, HANDLE hProcessHeap)
{
	g_ApcCounter.Completed = 0;
	g_ApcCounter.Fired = 0;
	g_opts.dirBufCallback = userCallback;
	g_opts.hProcessHeap = hProcessHeap;
}

void myNtEnumApcReportCounter(long* fired, long* completed)
{
	*fired = g_ApcCounter.Fired;
	*completed = g_ApcCounter.Completed;
}