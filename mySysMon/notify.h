#pragma once
#include <ntddk.h>

PCREATE_PROCESS_NOTIFY_ROUTINE_EX NotifyRoutine(
	 HANDLE ParentId,
	 HANDLE ProcessId,
	 PPS_CREATE_NOTIFY_INFO CreateInfo
);

PCREATE_THREAD_NOTIFY_ROUTINE ThreadRoutine(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create);

