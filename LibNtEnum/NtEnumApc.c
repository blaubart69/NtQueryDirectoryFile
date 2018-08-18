#include <ntifs.h>
#include <wdm.h>

#include "NtEnum.h"

#include "Win32.h"

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
	DECLSPEC_ALIGN(64) NTSTATUS							myNTSTATUS;
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
		ctx->myNTSTATUS = 0;

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

int writeOut(const WCHAR * format, ...);

void NTAPI NtQueryDirectoryApcCallback(_In_ PVOID ApcContext, _In_ PIO_STATUS_BLOCK IoStatusBlock, _In_ ULONG Reserved)
{
	writeOut(L"NtQueryDirectoryApcCallback: enter - fired %ld, completed %ld, NtStatus 0x%lX, IO.Information %lu\n", 
		g_ApcCounter.Fired, g_ApcCounter.Completed,
		IoStatusBlock->Status, IoStatusBlock->Information);

	MyApcContext *ctx = (MyApcContext*)ApcContext;

	if (ctx->myNTSTATUS == STATUS_NO_MORE_FILES)
	{
		NtClose(ctx->dirHandle);
		writeOut(L"NtQueryDirectoryApcCallback: exit because of myNO_MORE_FILES. dirHandle closed\n");
		return;
	}


	BOOLEAN ApcSubmittedOk = FALSE;

	if (!NT_SUCCESS(IoStatusBlock->Status))
	{
		if (IoStatusBlock->Status != STATUS_NO_MORE_FILES)
		{
			writeOut(L"NtQueryDirectoryApcCallback: !SUCCESS->!NO_MORE_FILES Information=%lu\n", IoStatusBlock->Information);
			g_opts.dirBufCallback(ctx->NtObjDirname, ctx->byteLenNtObjDirname, ctx->dirBuffer, FALSE, IoStatusBlock->Status, L"NtQueryDirectoryFile(callback != STATUS_NO_MORE_FILES)");
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

			writeOut(L"NtQueryDirectoryFile: fired because more data to come. NTSTATUS 0x%lX, IoStatus.Information %lu\n", ntStat, ctx->ioBlock.Information);

			if (NT_SUCCESS(ntStat))
			{
				writeOut(L"NtQueryDirectoryFile: internal NtQueryDirectoryFile SUCCESS\n");
				g_ApcCounter.Fired += 1;
				ApcSubmittedOk = TRUE;
			}
			else 
			{
				if (ntStat == STATUS_NO_MORE_FILES)
				{
					ctx->myNTSTATUS = STATUS_NO_MORE_FILES;
					writeOut(L"NtQueryDirectoryFile: settings myNTSTATUS to no_more_files\n");
				}
				else
				{
					g_opts.dirBufCallback(ctx->NtObjDirname, ctx->byteLenNtObjDirname, ctx->dirBuffer, FALSE, ntStat, L"NtQueryDirectoryFile(callback !NT_STATUS after call to NtQueryDirectoryFile)");
				}
			}
		}
	}

	if (!ApcSubmittedOk)
	{
		RtlFreeHeap(g_opts.hProcessHeap, 0, ApcContext);
	}

	g_ApcCounter.Completed += 1;
	writeOut(L"NtQueryDirectoryApcCallback: leave - fired %ld, completed %ld\n", g_ApcCounter.Fired, g_ApcCounter.Completed);
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
	//ntStat = myNtOpenFile(ctx->NtObjDirname, ctx->byteLenNtObjDirname, &dirHandle);
	int ok = myCreateFile(ctx->NtObjDirname, &dirHandle);
	if (!ok)
	{
		*NtApinameError = L"CreateFile";
		*ntstatus = -1;
		return FALSE;
	}
	{
		writeOut(L"CreateFile ok\n");
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
		, TRUE								// RestartScan
	);

	if (NT_SUCCESS(ntStat))
	{
		g_ApcCounter.Fired += 1;
		writeOut(L"ApcStart: NtStatus 0x%lX, Io.Info %lu\n", ntStat, ctx->ioBlock.Information);
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