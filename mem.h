#pragma once

#undef RtlMoveMemory
__declspec(dllimport) void __stdcall RtlMoveMemory(void *dst, const void *src, size_t len);

#undef RtlFillMemory
__declspec(dllimport) void __stdcall RtlFillMemory(void *dst, size_t len, unsigned char fill);

