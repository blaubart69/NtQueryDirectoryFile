#include "stdafx.h"

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
		if (!TryToSetPrivilege(SE_BACKUP_NAME, TRUE))
		{
			writeOut(L"could not set privilege SE_BACKUP_NAME");
		}

		GetFileInformationByHandleEx()

		WCHAR dirname[1024];
		int len = wsprintfW(dirname, L"\\\\?\\%s", args[1]);
		rc = RunEnumApc(dirname, len, TRUE) ? 0 : 8;
		//rc = RunEnumSeq(dirname, len) ? 0 : 8;
	}
	ExitProcess(rc);
}

