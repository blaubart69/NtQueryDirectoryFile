#include "stdafx.h"

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
typedef  struct _myApcContext
{
	DECLSPEC_ALIGN(8)
	union {
		FILE_DIRECTORY_INFORMATION	dirBuffer[1];
		char buf[4096];
	};
	IO_STATUS_BLOCK					ioBlock;
	HANDLE							dirHandle;
	USHORT							cbDirname;
	WCHAR							dirname[1];
} MyApcContext;
//
//
//

#undef RtlMoveMemory
__declspec(dllimport) void __stdcall RtlMoveMemory(void *dst, const void *src, size_t len);

MyApcContext* AllocContextStruct(LPCWSTR dirname, const USHORT byteLenNtObjDirname, const WCHAR* dirToAppendWithoutZero, const USHORT byteLenDirToAppendWithoutZero)
{
	int byteLenFullDir =
		byteLenNtObjDirname
		+ sizeof(WCHAR)			// L"\"
		+ byteLenDirToAppendWithoutZero
		+ sizeof(WCHAR)			// L"\0"
		;

	const DWORD DirNameOffset = FIELD_OFFSET(MyApcContext, dirname);
	const DWORD bytesToAlloc = byteLenFullDir + DirNameOffset;

	MyApcContext* newCtx = (MyApcContext*)HeapAlloc(g_opts.hProcessHeap, 0, bytesToAlloc);
	if (newCtx != NULL)
	{
		newCtx->ioBlock.Information = 0;
		newCtx->cbDirname = byteLenNtObjDirname;

		WCHAR* w = newCtx->dirname;
		MoveMemory(w, dirname, byteLenNtObjDirname + sizeof(WCHAR)); // copy with terminating zero

		if (byteLenDirToAppendWithoutZero > 0)
		{
			w += byteLenNtObjDirname / sizeof(WCHAR);
			//(char*)w += byteLenNtObjDirname;
			if (*(w-1) != L'\\')
			{
				*w++ = L'\\';
				newCtx->cbDirname += sizeof(WCHAR);
			}
			MoveMemory(w, dirToAppendWithoutZero, byteLenDirToAppendWithoutZero);
			w							  += byteLenDirToAppendWithoutZero / sizeof(WCHAR);
			//(char*)w += byteLenDirToAppendWithoutZero;
			*w = L'\0';
			newCtx->cbDirname	  += byteLenDirToAppendWithoutZero;
		}
	}

	return newCtx;
}

void NTAPI NtQueryDirectoryApcCallback(_In_ PVOID ApcContext, _In_ PIO_STATUS_BLOCK IoStatusBlock, _In_ ULONG Reserved)
{
	MyApcContext *ctx = (MyApcContext*)ApcContext;

#ifdef PRINT_DEBUG
	writeOut(L"   Apc_CallB: >>> enter - fired/completed/NtStatus/Info %5ld/%5ld/0x%08lX/%8lu | %s\n",
		g_ApcCounter.Fired, g_ApcCounter.Completed,
		IoStatusBlock->Status, IoStatusBlock->Information,
		ctx->dirname);
#endif

	if (!NT_SUCCESS(IoStatusBlock->Status))
	{
		if (IoStatusBlock->Status == STATUS_NO_MORE_FILES)
		{
			CloseHandle(ctx->dirHandle);
			HeapFree(g_opts.hProcessHeap, 0, ApcContext);
#ifdef PRINT_DEBUG
			writeOut(L"      Apc_CallB: STATUS_NO_MORE_FILES. CloseHandle(dirHandle); HeapFree(0x%lX);\n", ApcContext);
#endif
		}
		else
		{
#ifdef PRINT_DEBUG
			writeOut(L"      Apc_CallB: !SUCCESS->!NO_MORE_FILES Information=%lu\n", IoStatusBlock->Information);
#endif
			g_opts.dirBufCallback(ctx->dirname, ctx->cbDirname, ctx->dirBuffer, FALSE, IoStatusBlock->Status, L"NtQueryDirectoryFile(callback != STATUS_NO_MORE_FILES)");
		}
	}
	else
	{
		if (IoStatusBlock->Information != 0)
		{
			g_opts.dirBufCallback(ctx->dirname, ctx->cbDirname, ctx->dirBuffer, TRUE, IoStatusBlock->Status, NULL);

			NTSTATUS ntStat = NtQueryDirectoryFile(
				ctx->dirHandle
				, NULL								// event
				, NtQueryDirectoryApcCallback		// ApcRoutine
				, ctx								// ApcContext
				, &(ctx->ioBlock)
				, ctx->dirBuffer
				, 4096
				, FileDirectoryInformation
				, FALSE				// ReturnSingleEntry
				, NULL				// FileName
				, FALSE				// RestartScan
			);
			g_ApcCounter.Fired += 1;
#ifdef PRINT_DEBUG
			writeOut(L"      Apc_CallB: called NTSTATUS 0x%lX, IoStatus.Information %lu\n", ntStat, ctx->ioBlock.Information);
#endif

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
					g_opts.dirBufCallback(ctx->dirname, ctx->cbDirname, ctx->dirBuffer, FALSE, ntStat, L"NtQueryDirectoryFile(callback !NT_STATUS after call to NtQueryDirectoryFile)");
				}
			}
		}
	}

	g_ApcCounter.Completed += 1;
#ifdef PRINT_DEBUG
	writeOut(L"   Apc_CallB: <<< leave - fired/completed/NtStatus/Info %5ld/%5ld/0x%08lX/%8lu | %s\n",
		g_ApcCounter.Fired, g_ApcCounter.Completed,
		IoStatusBlock->Status, IoStatusBlock->Information,
		ctx->dirname);
#endif
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
	  LPCWSTR lpDirname
	, const USHORT byteDirnameLength
	, const WCHAR* dirToAppendWithoutZero
	, const USHORT byteLenDirToAppendWithoutZero
	, long* ntstatus
	, WCHAR** NtApinameError)
{
	NTSTATUS ntStat;

	MyApcContext* ctx = AllocContextStruct(lpDirname, byteDirnameLength, dirToAppendWithoutZero, byteLenDirToAppendWithoutZero);
	if (ctx == NULL)
	{
		*NtApinameError = L"AllocContextStruct";
		*ntstatus = -1;
		return FALSE;
	}

#ifdef PRINT_DEBUG
	writeOut(L"Apc_Start: parent/bytes: [%s]/%d, dir/bytes: [%s]/%d, new/bytes: [%s]/%d\n",
		lpDirname, byteDirnameLength, dirToAppendWithoutZero, byteLenDirToAppendWithoutZero,
		ctx->dirname, ctx->byteLenNtObjDirname);

	writeChars(ctx->dirname, ctx->byteLenNtObjDirname / 2);
#endif
	//
	//
	//
	HANDLE dirHandle = CreateFile(
			ctx->dirname
			, GENERIC_READ
			, FILE_SHARE_READ
			, NULL
			, OPEN_EXISTING
			, FILE_FLAG_OVERLAPPED | FILE_FLAG_BACKUP_SEMANTICS
			, NULL);

	if (dirHandle == INVALID_HANDLE_VALUE)
	{
		*NtApinameError = L"CreateFile";
		*ntstatus = -1;
		writeOut(L"E: CreateFile failed. [%s]\n", ctx->dirname);
		return FALSE;
	}
	//
	//
	//
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
#ifdef PRINT_DEBUG
		writeOut(L"Apc_Start: NtStatus 0x%lX, Io.Info %lu, addrCtx 0x%lX, byteLenDirToAppend %d, [%s]\n", 
			ntStat, ctx->ioBlock.Information, ctx, byteLenDirToAppendWithoutZero, ctx->dirname);
#endif
	}	
	else
	{
		HeapFree(g_opts.hProcessHeap, 0, ctx);
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