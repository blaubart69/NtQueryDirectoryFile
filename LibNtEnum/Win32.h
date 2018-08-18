#ifndef DECLSPEC_IMPORT
#if (defined(_M_IX86) || defined(_M_IA64) || defined(_M_AMD64) || defined(_M_ARM) || defined(_M_ARM64)) && !defined(MIDL_PASS)
#define DECLSPEC_IMPORT __declspec(dllimport)
#else
#define DECLSPEC_IMPORT
#endif
#endif

#define WINBASEAPI DECLSPEC_IMPORT

#define WINAPI				__stdcall
typedef void*				HANDLE;
typedef unsigned long       DWORD;
typedef int                 BOOL;

typedef struct _SECURITY_ATTRIBUTES {
	DWORD nLength;
	void* lpSecurityDescriptor;
	BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

#define OPEN_EXISTING       3
#define FILE_FLAG_OVERLAPPED            0x40000000
#define FILE_FLAG_BACKUP_SEMANTICS      0x02000000

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

WINBASEAPI
HANDLE
WINAPI
CreateFileW(
	_In_ LPCWSTR lpFileName,
	_In_ DWORD dwDesiredAccess,
	_In_ DWORD dwShareMode,
	_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	_In_ DWORD dwCreationDisposition,
	_In_ DWORD dwFlagsAndAttributes,
	_In_opt_ HANDLE hTemplateFile
);

WINBASEAPI
BOOL
WINAPI
CancelIo(
	_In_ HANDLE hFile
);
