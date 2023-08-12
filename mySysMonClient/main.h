#pragma once

void DisplayInfo(BYTE* buffer, DWORD size);

void DisplayTime(const LARGE_INTEGER& time);

int HandleProcessExit(ItemHeader* header, BYTE* buffer);

int HandleProcessCreate(ItemHeader* header, BYTE* buffer);

int HandleThreadDelete(ItemHeader* header, BYTE* buffer);

int HandleThreadCreate(ItemHeader* header, BYTE* buffer);

WCHAR* GetImageName(HANDLE handle, ThreadInfo* info);

BOOL isThreadRemote(HANDLE handle, ThreadInfo* info);
