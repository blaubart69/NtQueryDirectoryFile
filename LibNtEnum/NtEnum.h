#pragma once

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

typedef void(*DirBufferCallback)(SPI_FILE_DIRECTORY_INFORMATION* FileInfoBuffer);

long myNtEnum(LPCWSTR NtObjDirname, USHORT byteDirnameLength, USHORT byteBufLength, DirBufferCallback dirBufCallback);
