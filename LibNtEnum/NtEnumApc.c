#include <ntifs.h>
#include <wdm.h>

#include "NtEnum.h"

#include "Win32.h"

NTSTATUS myNtOpenFile(LPCWSTR NtObjDirname, USHORT byteDirnameLength, PHANDLE dirHandle);

typedef struct _ApcCounter
{
	DECLSPEC_ALIGN(8) long Fired;
	DECLSPEC_ALIGN(8) long Completed;
} ApcCounter;

typedef struct _EnumApcOptions
{
	pfDirBufferApc dirBufCallback;
	HANDLE hProcessHeap;
} EnumApcOptions;

ApcCounter		g_ApcCounter;
EnumApcOptions	g_opts;

//const SIZE_T dirBufSize = 4096;

//
// APC context
//
typedef struct _myApcContext
{
	union {
		SPI_FILE_DIRECTORY_INFORMATION	dirBuffer[1];
		char buff[4096];
	};
	IO_STATUS_BLOCK					ioBlock;
	HANDLE							dirHandle;
	USHORT							byteLenNtObjDirname;
	WCHAR							NtObjDirname[1];
} MyApcContext;
//
//
//
MyApcContext* AllocContextStruct(LPCWSTR dirname, const USHORT byteLenNtObjDirname, const WCHAR* dirToAppendWithoutZero, const USHORT byteLenDirToAppendWithoutZero)
{
	int byteLenFullDir =
		byteLenNtObjDirname
		+ sizeof(WCHAR)			// L"\"
		+ byteLenDirToAppendWithoutZero
		+ sizeof(WCHAR)			// L"\0"
		;

	MyApcContext* newCtx = (MyApcContext*)
		RtlAllocateHeap(
			g_opts.hProcessHeap, 
			0, 
			  byteLenFullDir
			+ FIELD_OFFSET(MyApcContext, NtObjDirname));

	if (newCtx != NULL)
	{
		newCtx->ioBlock.Information = 0;
		newCtx->byteLenNtObjDirname = byteLenNtObjDirname;

		WCHAR* w = newCtx->NtObjDirname;
		RtlMoveMemory(w, dirname, byteLenNtObjDirname + sizeof(WCHAR)); // copy with terminating zero

		if (byteLenDirToAppendWithoutZero > 0)
		{
			w += byteLenNtObjDirname / sizeof(WCHAR);
			//(char*)w += byteLenNtObjDirname;
			if (*(w-1) != L'\\')
			{
				*w++ = L'\\';
				newCtx->byteLenNtObjDirname += sizeof(WCHAR);
			}
			RtlMoveMemory(w, dirToAppendWithoutZero, byteLenDirToAppendWithoutZero);
			w							  += byteLenDirToAppendWithoutZero / sizeof(WCHAR);
			//(char*)w += byteLenDirToAppendWithoutZero;
			*w = L'\0';
			newCtx->byteLenNtObjDirname	  += byteLenDirToAppendWithoutZero;
		}
	}

	return newCtx;
}

int writeOut(const WCHAR * format, ...);

void NTAPI NtQueryDirectoryApcCallback(_In_ PVOID ApcContext, _In_ PIO_STATUS_BLOCK IoStatusBlock, _In_ ULONG Reserved)
{
	MyApcContext *ctx = (MyApcContext*)ApcContext;

	writeOut(L"   Apc_CallB: >>> enter - fired/completed/NtStatus/Info %5ld/%5ld/0x%08lX/%8lu | %s\n",
		g_ApcCounter.Fired, g_ApcCounter.Completed,
		IoStatusBlock->Status, IoStatusBlock->Information,
		ctx->NtObjDirname);

	if (!NT_SUCCESS(IoStatusBlock->Status))
	{
		if (IoStatusBlock->Status == STATUS_NO_MORE_FILES)
		{
			NtClose(ctx->dirHandle);
			RtlFreeHeap(g_opts.hProcessHeap, 0, ApcContext);
			writeOut(L"      Apc_CallB: STATUS_NO_MORE_FILES. NtClose(dirHandle); RtlFreeHeap(0x%lX);\n", ApcContext);
		}
		else
		{
			writeOut(L"      Apc_CallB: !SUCCESS->!NO_MORE_FILES Information=%lu\n", IoStatusBlock->Information);
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
				, 4096
				, FileDirectoryInformation
				, FALSE				// ReturnSingleEntry
				, NULL				// FileName
				, FALSE				// RestartScan
			);
			g_ApcCounter.Fired += 1;

			writeOut(L"      Apc_CallB: called NTSTATUS 0x%lX, IoStatus.Information %lu\n", ntStat, ctx->ioBlock.Information);

			if (NT_SUCCESS(ntStat))
			{
				//writeOut(L"NtQueryDirectoryFile: internal NtQueryDirectoryFile SUCCESS\n");
			}
			else
			{
				if (ntStat == STATUS_NO_MORE_FILES)
				{
					//ctx->myNTSTATUS = STATUS_NO_MORE_FILES;
					//writeOut(L"NtQueryDirectoryFile: settings myNTSTATUS to no_more_files\n");
				}
				else
				{
					g_opts.dirBufCallback(ctx->NtObjDirname, ctx->byteLenNtObjDirname, ctx->dirBuffer, FALSE, ntStat, L"NtQueryDirectoryFile(callback !NT_STATUS after call to NtQueryDirectoryFile)");
				}
			}
		}
	}

	g_ApcCounter.Completed += 1;
	writeOut(L"   Apc_CallB: <<< leave - fired/completed/NtStatus/Info %5ld/%5ld/0x%08lX/%8lu | %s\n",
		g_ApcCounter.Fired, g_ApcCounter.Completed,
		IoStatusBlock->Status, IoStatusBlock->Information,
		ctx->NtObjDirname);
}

void writeChars(LPCWSTR chars, const int lenChars)
{
	for (int i = 0; i < lenChars; i++)
	{
		writeOut(L"%C ", chars[i]);
	}
	writeOut(L"\n");

	for (int i = 0; i < lenChars; i++)
	{
		writeOut(L"%02X", chars[i]);
	}
	writeOut(L"\n");
}

int myNtEnumApcStart(
	  LPCWSTR NtObjDirname
	, const USHORT byteDirnameLength
	, const WCHAR* dirToAppendWithoutZero
	, const USHORT byteLenDirToAppendWithoutZero
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
	
	long x = FIELD_OFFSET(MyApcContext, dirBuffer);

	writeOut(L"Apc_Start: parent/bytes: [%s]/%d, dir/bytes: [%s]/%d, new/bytes: [%s]/%d\n",
		NtObjDirname, byteDirnameLength, dirToAppendWithoutZero, byteLenDirToAppendWithoutZero,
		ctx->NtObjDirname, ctx->byteLenNtObjDirname);

	writeChars(ctx->NtObjDirname, ctx->byteLenNtObjDirname / 2);

	HANDLE dirHandle;
	//ntStat = myNtOpenFile(ctx->NtObjDirname, ctx->byteLenNtObjDirname, &dirHandle);
	int ok = myCreateFile(ctx->NtObjDirname, &dirHandle);
	if (!ok)
	{
		*NtApinameError = L"CreateFile";
		*ntstatus = -1;
		writeOut(L"E: CreateFile failed. [%s]\n", ctx->NtObjDirname);
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
		, 4096
		, FileDirectoryInformation
		, FALSE								// ReturnSingleEntry
		, NULL								// FileName
		, TRUE								// RestartScan
	);
	g_ApcCounter.Fired += 1;

	if (NT_SUCCESS(ntStat))
	{
		writeOut(L"Apc_Start: NtStatus 0x%lX, Io.Info %lu, addrCtx 0x%lX, byteLenDirToAppend %d, [%s]\n", 
			ntStat, ctx->ioBlock.Information, ctx, byteLenDirToAppendWithoutZero, ctx->NtObjDirname);
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