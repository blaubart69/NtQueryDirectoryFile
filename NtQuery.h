#pragma once

//#define WIN32_LEAN_AND_MEAN
//#include <Windows.h>

int writeOut(const WCHAR * format, ...);
void printFileEntries(FILE_DIRECTORY_INFORMATION *dirBuffer);
BOOL writeOut(const WCHAR * format, ...);
void WriteNtError(long ntStatus, LPCWSTR NtApinameError, LPCWSTR Functionname);
BOOL IsDotDir(LPCWSTR cFileName, const DWORD cFilenameLenBytes, const DWORD dwFileAttributes);
BOOL TryToSetPrivilege(LPCWSTR szPrivilege, BOOL bEnablePrivilege);

//BOOL RunEnumSeq(LPCWSTR dirname, int NtObjDirnameLen);
BOOL RunEnumApc(LPCWSTR dirname, int NtObjDirnameLen, BOOL sum);

//typedef void(*pfDirBufferCallback)(FILE_DIRECTORY_INFORMATION* FileInfoBuffer);
typedef void(*pfDirBufferApc)     (LPCWSTR NtObjDirname, const USHORT byteDirnameLength, const FILE_DIRECTORY_INFORMATION* FileInfoBuffer, const int success, const long ntStatus, const WCHAR* NtApinameError);

/*
long myNtEnumSeq(LPCWSTR NtObjDirname, USHORT byteDirnameLength, pfDirBufferCallback dirBufCallback
, PVOID bufferToUse, unsigned long bufferSize, WCHAR** NtApinameError);
*/

void myNtEnumApcInit(pfDirBufferApc userCallback, HANDLE hProcessHeap);
void myNtEnumApcReportCounter(long* fired, long* completed);
int myNtEnumApcStart(
	LPCWSTR NtObjDirname
	, const USHORT byteDirnameLength
	, const WCHAR* dirToAppendWithoutZero
	, const USHORT byteLenDirToAppendWithoutZero
	, long* ntstatus
	, WCHAR** NtApinameError);