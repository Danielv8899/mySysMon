#include "main.h"
#include "common.h"
#include "ioctl.h"
#include "mem.h"

Globals g_Globals;

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING RegistryPath)
{	

	auto status = STATUS_SUCCESS;
	InitializeListHead(&g_Globals.ListHead);
	g_Globals.Mutex.init();

	OBJECT_ATTRIBUTES attributes;
	HANDLE hKey;
	UNICODE_STRING valueName;
	ULONG value;
	PKEY_VALUE_FULL_INFORMATION valInfo = NULL;

	RtlInitUnicodeString(&valueName, L"ItemLimit");

	KdPrint(("RegPath : %wZ\n", RegistryPath));

	InitializeObjectAttributes(&attributes, RegistryPath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

	status = ZwOpenKey(&hKey, KEY_ALL_ACCESS, &attributes);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("DriverEntry: ZwOpenKey failed on ROOT: %x\n", status));
		return status;
	}
	ULONG size = 0;
	ULONG sizeNeeded = 0;
	status = ZwQueryValueKey(hKey, &valueName, KeyValueFullInformation,valInfo,size,&sizeNeeded);
	if ((status == STATUS_BUFFER_TOO_SMALL) || (status == STATUS_BUFFER_OVERFLOW))
	{
		KdPrint(("Buffer size is %x\n", sizeNeeded));
		size = sizeNeeded;
		valInfo = (PKEY_VALUE_FULL_INFORMATION)malloc(size, NonPagedPool);
		if (!valInfo) {
			KdPrint(("Failed allocating valInfo\n"));
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		KdPrint(("Allocated valInfo\n"));
		RtlZeroMemory(valInfo, size);
		status = ZwQueryValueKey(hKey, &valueName, KeyValueFullInformation, valInfo, size, &sizeNeeded);
		if (status == STATUS_INVALID_HANDLE) {
			value = 1024;
			KdPrint(("DriverEntry: ZwQueryValueKey failed: %x\n Creating key.\n", status));
			status = ZwSetValueKey(hKey, &valueName, 0, REG_DWORD, &value, sizeof(value));
			if (!NT_SUCCESS(status))
			{
				KdPrint(("DriverEntry: ZwSetValueKey failed: %x\n", status));
				return status;
			}

		}
		else if (!NT_SUCCESS(status) || (size != sizeNeeded) || (valInfo == NULL))
		{
			KdPrint(("DriverEntry: ZwQueryValueKey failed: %x\n", status));
			return status;
		}
	}
	else
	{
		KdPrint(("DriverEntry: ZwQueryValueKey failed: %x\n", status));
		return status;
	}

	KdPrint(("Datalen: %x\n", valInfo->DataLength));
	auto valuePtr = (DWORD32*)((UCHAR*)valInfo + valInfo->DataOffset);
	//RtlCopyMemory(&g_Globals.ItemLimit, valuePtr, valInfo->DataLength);
	g_Globals.ItemLimit = *valuePtr;

	KdPrint(("ItemLimit: %x\n", g_Globals.ItemLimit));

	PDEVICE_OBJECT pDeviceObject = NULL;
	UNICODE_STRING deviceName, deviceSymLink;

	RtlInitUnicodeString(&deviceName, L"\\Device\\mySysMon");
	RtlInitUnicodeString(&deviceSymLink, L"\\??\\mySysMon");

	status = IoCreateDevice(pDriverObject, 0, &deviceName, 0, 0, 0, &pDeviceObject);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("DriverEntry: IoCreateDevice failed: %x\n", status));
		return status;
	}

	StartRecordingRoutine();

	pDriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCloseRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateCloseRoutine;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoctlRoutine;

	pDriverObject->MajorFunction[IRP_MJ_READ] = ReadRoutine;
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = WriteRoutine;

	pDriverObject->DriverUnload = DriverUnload;

	pDeviceObject->Flags |= DO_DIRECT_IO;

	status = IoCreateSymbolicLink(&deviceSymLink, &deviceName);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("DriverEntry: IoCreateSymbolicLink failed: %x\n", status));
		IoDeleteDevice(pDeviceObject);
		return status;
	}

	return status;
}

NTSTATUS ReadRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp) {
	UNREFERENCED_PARAMETER(pDeviceObject);
	auto stack = IoGetCurrentIrpStackLocation(pIrp);
	auto len = stack->Parameters.Read.Length;
	auto status = STATUS_SUCCESS;
	auto count = 0;
	NT_ASSERT(pIrp->MdlAddress);
	//KdBreakPoint();

	auto buffer = (UCHAR*)MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);
	if (!buffer) {
		KdPrint(("Failed getting buffer\n"));
		status = STATUS_INSUFFICIENT_RESOURCES;
		return CompleteRequest(pIrp, status, 0);
	}

	AutoLock<MyFastMutex> lock(g_Globals.Mutex);
	while (true) {
		if (IsListEmpty(&g_Globals.ListHead)) {
			break;
		}

		auto entry = RemoveHeadList(&g_Globals.ListHead);
		auto info = CONTAINING_RECORD(entry, LItem<ItemHeader>, ListEntry);
		auto size = info->Item.Size;
		if (len < size) {
			InsertHeadList(&g_Globals.ListHead, entry);
			break;
		}
		g_Globals.ItemCount--;
		RtlCopyMemory(buffer, &info->Item, size);
		len -= size;
		buffer += size;
		count += size;
		delete info;
	}

	return CompleteRequest(pIrp, status, count);
}

NTSTATUS WriteRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp) {
	UNREFERENCED_PARAMETER(pDeviceObject);
	return CompleteRequest(pIrp, STATUS_SUCCESS, 0);
}

NTSTATUS CreateCloseRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDeviceObject);
	return CompleteRequest(pIrp, STATUS_SUCCESS, 0);
}

NTSTATUS IoctlRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp) {

	UNREFERENCED_PARAMETER(pDeviceObject);
	
	auto stack = IoGetCurrentIrpStackLocation(pIrp);
	auto status = STATUS_SUCCESS;
	auto info = 0;

	switch (stack->Parameters.DeviceIoControl.IoControlCode) 
	{
		case IOCTL_START_RECORDING:
			KdPrint(("IOCTL_START_RECORDING called\n"));
			status = StartRecordingRoutine();
			break;

		case IOCTL_STOP_RECORDING:
			KdPrint(("IOCTL_STOP_RECORDING called\n"));
			status = StopRecordingRoutine();
			break;

		default:
			KdPrint(("IOCTL unknown\n"));
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;
	}

	return CompleteRequest(pIrp, status, info);
}

void DriverUnload(PDRIVER_OBJECT pDriverObject) {

	UNICODE_STRING deviceSymLink;
	StopRecordingRoutine();
	RtlInitUnicodeString(&deviceSymLink, L"\\??\\mySysMon");
	IoDeleteSymbolicLink(&deviceSymLink);
	IoDeleteDevice(pDriverObject->DeviceObject);

	while (!IsListEmpty(&g_Globals.ListHead)) {
		auto entry = RemoveHeadList(&g_Globals.ListHead);
		auto info = CONTAINING_RECORD(entry, LItem<ItemHeader>, ListEntry);
		//KdBreakPoint();
		delete info;
	}
}

NTSTATUS CompleteRequest(PIRP pIrp, NTSTATUS status, ULONG_PTR info) {
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = info;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}

void PushItem(LIST_ENTRY* item)
{
	AutoLock<MyFastMutex> lock(g_Globals.Mutex);
	if (g_Globals.ItemCount >= g_Globals.ItemLimit)
	{
		LIST_ENTRY* lastItem = RemoveHeadList(&g_Globals.ListHead);
		g_Globals.ItemCount--;
		//auto itemLen = reinterpret_cast<LItem<ItemHeader>*>(lastItem)->Item.Size;
		//RtlZeroMemory(lastItem, itemLen);
		//RtlZeroMemory(lastItem, sizeof(LIST_ENTRY));
		delete lastItem;
	}
	InsertHeadList(&g_Globals.ListHead, item);
	g_Globals.ItemCount++;
}