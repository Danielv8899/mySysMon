#include "notify.h"
#include "common.h"
#include "mem.h"
#include "main.h"

PCREATE_PROCESS_NOTIFY_ROUTINE_EX NotifyRoutine(HANDLE ParentId, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	if (CreateInfo)
	{
		KdPrint(("Notify: %d %d %wZ Created by Process: %d, Thread ID %d \n", ParentId, ProcessId, CreateInfo->ImageFileName, CreateInfo->CreatingThreadId.UniqueProcess, CreateInfo->CreatingThreadId.UniqueThread));
		
		USHORT allocSize = sizeof(LItem<ProcessCreateInfo>);
		USHORT commandLineSize = 0;
		USHORT ImageSize = 0;
		if (CreateInfo->CommandLine)
		{
			commandLineSize = CreateInfo->CommandLine->Length;
			allocSize += commandLineSize;
		}
		if (CreateInfo->ImageFileName && CreateInfo->ImageFileName->Length > 1) {
			ImageSize = CreateInfo->ImageFileName->Length;
			allocSize += ImageSize;
		}
		auto info = (LItem<ProcessCreateInfo>*)malloc(allocSize, NonPagedPool);
		if (info == nullptr) {
			KdPrint(("Notify: Failed to allocate memory for LItem<ProcessCreateInfo> \n"));
			return 0;
		}

		auto& item = info->Item;
		KeQuerySystemTimePrecise(&item.Time);
		item.Type = ItemType::ProcessCreate;
		item.Size = allocSize;
		item.ProcessId = HandleToUlong(ProcessId);
		item.ParentProcessId = HandleToUlong(CreateInfo->ParentProcessId);
		if (commandLineSize > 0)
		{
			RtlCopyMemory((UCHAR*)&item + sizeof(item), CreateInfo->CommandLine->Buffer, commandLineSize);
			item.CommandLineLength = commandLineSize / sizeof(WCHAR);
			item.CommandLineOffset = sizeof(item);
		}

		else {
			item.CommandLineLength = 0;
			item.CommandLineOffset = 0;
		}

		if (ImageSize > 0) {
			RtlCopyMemory((UCHAR*)&item + sizeof(item) + commandLineSize, CreateInfo->ImageFileName->Buffer, ImageSize);
			item.ImageNameLength = ImageSize / sizeof(WCHAR);
			item.ImageNameOffset = sizeof(item) + commandLineSize;
		}
		else {
			item.ImageNameLength = 0;
			item.ImageNameOffset = 0;
		}

		PushItem(&info->ListEntry);
	}

	else
	{
		KdPrint(("Notify: %d %d \n", ParentId, ProcessId));
		auto info = new (NonPagedPool) LItem<ProcessExitInfo>;
		if (!info) {
			KdPrint(("Notify: Failed to allocate memory for LItem<ProcessExitInfo> \n"));
			return 0;
		}
		auto& item = info->Item;
		KeQuerySystemTimePrecise(&item.Time);
		item.Type = ItemType::ProcessExit;
		item.Size = sizeof(ProcessExitInfo);
		item.ProcessId = HandleToULong(ProcessId);
		PushItem(&info->ListEntry);
	}
	return 0;
}

PCREATE_THREAD_NOTIFY_ROUTINE ThreadRoutine(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create) {

	if (Create) { //thread created
		KdPrint(("Notify: Thread created: pid: %d, tid: %d\n", ProcessId, ThreadId));

		USHORT allocSize = sizeof(LItem<ThreadInfo>);

		auto info = (LItem<ThreadInfo>*)malloc(allocSize, NonPagedPool);
		if (info == nullptr) {
			KdPrint(("Notify: failed to allocate memory for LItem<ThreadInfo>"));
			return 0;
		}

		auto& item = info->Item;
		KeQuerySystemTimePrecise(&item.Time);
		item.Type = ItemType::ThreadCreate;
		item.Size = allocSize;
		item.ProcessId = HandleToUlong(ProcessId);
		item.ThreadId = HandleToUlong(ThreadId);

		PushItem(&info->ListEntry);
	}

	else { //thread deleted
		KdPrint(("Notify: Thread deleted: pid: %d, tid: %d\n", ProcessId, ThreadId));

		USHORT allocSize = sizeof(LItem<ThreadInfo>);

		auto info = (LItem<ThreadInfo>*)malloc(allocSize, NonPagedPool);
		if (info == nullptr) {
			KdPrint(("Notify: failed to allocate memory for LItem<ThreadInfo>"));
			return 0;
		}

		auto& item = info->Item;
		KeQuerySystemTimePrecise(&item.Time);
		item.Type = ItemType::ThreadDelete;
		item.Size = allocSize;
		item.ProcessId = HandleToUlong(ProcessId);
		item.ThreadId = HandleToUlong(ThreadId);

		PushItem(&info->ListEntry);
	}
	return 0;
}