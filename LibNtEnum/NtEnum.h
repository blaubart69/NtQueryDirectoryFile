#pragma once

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct _SPI_FILE_DIRECTORY_INFORMATION {
		unsigned long NextEntryOffset;
		unsigned long FileIndex;
		__int64 CreationTime;
		__int64 LastAccessTime;
		__int64 LastWriteTime;
		__int64 ChangeTime;
		__int64 EndOfFile;
		__int64 AllocationSize;
		unsigned long FileAttributes;
		unsigned long FileNameLength;
		wchar_t FileName[1];
	} SPI_FILE_DIRECTORY_INFORMATION, *PSPI_FILE_DIRECTORY_INFORMATION;

	typedef void(*pfDirBufferCallback)(SPI_FILE_DIRECTORY_INFORMATION* FileInfoBuffer);
	typedef void(*pfDirBufferApc)     (SPI_FILE_DIRECTORY_INFORMATION* FileInfoBuffer, int success, long ntStatus, WCHAR* NtApinameError);

	long myNtEnum(LPCWSTR NtObjDirname, USHORT byteDirnameLength, pfDirBufferCallback dirBufCallback
		, PVOID bufferToUse, unsigned long bufferSize, WCHAR** NtApinameError);

	int
		myNtEnumApcStart(LPCWSTR NtObjDirname, USHORT byteDirnameLength
			, pfDirBufferApc dirBufCallback
			, HANDLE hProcessHeap
			, long* ntstatus
			, WCHAR** NtApinameError);

#ifdef __cplusplus
}
#endif