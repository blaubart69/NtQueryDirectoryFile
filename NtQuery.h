#pragma once

//#define WIN32_LEAN_AND_MEAN
//#include <Windows.h>

int writeOut(const WCHAR * format, ...);
void printFileEntries(FILE_DIRECTORY_INFORMATION *dirBuffer);
BOOL writeOut(const WCHAR * format, ...);
void WriteNtError(long ntStatus, LPCWSTR NtApinameError, LPCWSTR Functionname);
BOOL IsDotDir(LPCWSTR cFileName, const DWORD cFilenameLenBytes, const DWORD dwFileAttributes);

//BOOL RunEnumSeq(LPCWSTR dirname, int NtObjDirnameLen);
BOOL RunEnumApc(LPCWSTR dirname, int NtObjDirnameLen);