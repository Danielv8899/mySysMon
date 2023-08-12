#pragma once
#include <ntddk.h>
#include "sync.h"

NTSTATUS ReadRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);

NTSTATUS WriteRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);

NTSTATUS CreateCloseRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);

NTSTATUS IoctlRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);

void DriverUnload(PDRIVER_OBJECT pDriverObject);

NTSTATUS CompleteRequest(PIRP pIrp, NTSTATUS status, ULONG_PTR info);

void PushItem( LIST_ENTRY* item);

struct Globals {
	LIST_ENTRY ListHead;
	int ItemCount;
	MyFastMutex Mutex;
	int ItemLimit;
};