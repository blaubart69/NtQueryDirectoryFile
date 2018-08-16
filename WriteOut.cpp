#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

BOOL writeOut(const WCHAR * format, ...)
{
	va_list args;
	va_start(args, format);

	WCHAR buf[1024];
	const int writtenChars = wvsprintfW(buf, format, args);
	va_end(args);

	DWORD written;
	return WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), buf, writtenChars, &written, NULL);
}