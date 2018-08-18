#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#include "LibNtEnum/NtEnum.h"
#include "NtQuery.h"

void rawmain(void)
{
	int argc;
	LPWSTR *args = CommandLineToArgvW(GetCommandLineW(), &argc);
	UINT rc;

	if (argc != 2)
	{
		writeOut(L"usage: NtFind {dir}");
		rc = 1;
	}
	else
	{
		WCHAR dirname[1024];
		int len = wsprintfW(dirname, L"\\\\?\\%s", args[1]);
		rc = RunEnumApc(dirname, len) ? 0 : 8;
		//rc = RunEnumSeq(dirname, len) ? 0 : 8;
	}
	ExitProcess(rc);
}

