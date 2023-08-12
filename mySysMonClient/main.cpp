#include <windows.h>
#include <winternl.h>
#include <stdio.h>
#include <iostream>
#include "../mySysMon/common.h"
#include "main.h"

#pragma warning(disable:6262)
typedef NTSTATUS(NTAPI* signatureNtQueryInformationThread)(HANDLE, THREADINFOCLASS, PVOID, ULONG, PULONG);

typedef struct _THREAD_BASIC_INFORMATION
{
	NTSTATUS ExitStatus;
	PTEB TebBaseAddress;
	CLIENT_ID ClientId;
	ULONG_PTR AffinityMask;
	KPRIORITY Priority;
	LONG BasePriority;
} THREAD_BASIC_INFORMATION, * PTHREAD_BASIC_INFORMATION;

//typedef enum _THREADINFOCLASSTAK {

//	ThreadBasicInformation,
//	ThreadTimes,
//	ThreadPriority,
//	ThreadBasePriority,
//	ThreadAffinityMask,
//	ThreadImpersonationToken,
//	ThreadDescriptorTableEntry,
//	ThreadEnableAlignmentFaultFixup,
//	ThreadEventPair,
//	ThreadQuerySetWin32StartAddress,
//	ThreadZeroTlsCell,
//	ThreadPerformanceCount,
//	ThreadAmILastThread,
//	ThreadIdealProcessor,
//	ThreadPriorityBoost,
//	ThreadSetTlsArrayAddress,
//	ThreadIsIoPending,
//	ThreadHideFromDebugger,
//	ThreadBreakOnTermination,
//	ThreadSwitchLegacyState,
//	ThreadIsTerminated,
//	ThreadLastSystemCall,
//	ThreadIoPriority,
//	ThreadCycleTime,
//	ThreadPagePriority,
//	ThreadActualBasePriority,
//	ThreadTebInformation,
//	ThreadCSwitchMon,
//	ThreadCSwitchPmu,
//	ThreadWow64Context,
//	ThreadGroupInformation,
//	ThreadUmsInformation,
//	ThreadCounterProfiling,
//	ThreadIdealProcessorEx,
//	ThreadCpuAccountingInformation,
//	ThreadSuspendCount,
//	ThreadHeterogeneousCpuPolicy,
//	ThreadContainerId,
//	ThreadNameInformation,
//	ThreadProperty,
//	ThreadSelectedCpuSets,
//	ThreadSystemThreadInformation,
//	MaxProcessInfoClass
//	
//};


signatureNtQueryInformationThread myNtQueryInformationThread = nullptr;


int main(int argc, char* argv[]) {
	if (argc != 2) {
		printf("usage: %s <start|stop|monitor>\n", argv[0]);
	}

	auto deviceHandle = CreateFile(L"\\\\.\\mySysMon", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (deviceHandle == INVALID_HANDLE_VALUE) {
		printf("CreateFile failed: %d\n", GetLastError());
		return 1;
	}

	DWORD bytesReturned = 0;

	if (strcmp(argv[1], "start") == 0) {
		if (!DeviceIoControl(deviceHandle, IOCTL_START_RECORDING, NULL, 0, NULL, 0, &bytesReturned, NULL)) {
			printf("DeviceIoControl failed: %d\n", GetLastError());
			return 1;
		}
		printf("Started recording\n");
	}
	else if (strcmp(argv[1], "stop") == 0) {
		if (!DeviceIoControl(deviceHandle, IOCTL_STOP_RECORDING, NULL, 0, NULL, 0, &bytesReturned, NULL)) {
			printf("DeviceIoControl failed: %d\n", GetLastError());
			return 1;
		}
		printf("Stopped recording\n");
	}

	else if (strcmp(argv[1], "monitor") == 0) {
		BYTE buf[1 << 16];
		while (true) {
			DWORD bytes = 0;
			if (!ReadFile(deviceHandle, buf, sizeof(buf), &bytes, NULL)) {
				printf("ReadFile failed: %d\n", GetLastError());
				return 1;
			}

			if (bytes) {
				DisplayInfo(buf, bytes);
			}
			Sleep(200);
		}
	}
	return 0;
}

void DisplayInfo(BYTE* buffer, DWORD size) {
	auto count = size;
	std::wstring commandLine;
	//std::wstring imageName;
	while (count > 0) {
		auto header = (ItemHeader*)buffer;

		switch (header->Type) {
		case ItemType::ProcessCreate: {
			if (HandleProcessCreate( header, buffer))break;
		}
		//case ItemType::ProcessExit: {
		//	if (HandleProcessExit( header, buffer)) break;
		//}
		case ItemType::ThreadCreate: {
			if(HandleThreadCreate(header, buffer)) break;
		}

		//case ItemType::ThreadDelete: {
		//	if (HandleThreadDelete(header, buffer)) break;
		//}
			default:
				break;
		}
		buffer += header->Size;
		count -= header->Size;
	}
}

void DisplayTime(const LARGE_INTEGER& time) {
	SYSTEMTIME st;
	FileTimeToSystemTime((FILETIME*)&time, &st);
	printf("%02d:%02d:%02d.%03d: ",
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

int HandleProcessExit(ItemHeader* header, BYTE* buffer) {
	DisplayTime(header->Time);
	auto exitInfo = (ProcessExitInfo*)buffer;
	printf("Process %d exited\n", exitInfo->ProcessId);
	return 0;
}

int HandleProcessCreate(ItemHeader* header, BYTE* buffer) {
	DisplayTime(header->Time);
	auto info = (ProcessCreateInfo*)buffer;
	std::wstring commandLine((WCHAR*)(buffer + info->CommandLineOffset), info->CommandLineLength);
	std::wstring imageName((WCHAR*)(buffer + info->ImageNameOffset), info->ImageNameLength);
	printf("Process %d created: \nImage: %ws\ncmdLine: %ws \n", info->ProcessId, imageName.c_str(), commandLine.c_str()); //imageName.c_str(), commandLine.c_str());
	return 0;
}

int HandleThreadDelete(ItemHeader* header, BYTE* buffer) {
	DisplayTime(header->Time);
	auto info = (ThreadInfo*)buffer;
	printf("Process %d: Thread %d deleted\n", info->ProcessId, info->ThreadId);
	return 0;
}

int HandleThreadCreate(ItemHeader* header, BYTE* buffer) {

	auto info = (ThreadInfo*)buffer;

	auto kernelHandle = GetModuleHandle(L"ntdll.dll");
	if (!kernelHandle) {
		printf("getting handle to ntdll failed\n");
		return 1;
	}

	auto pNtQueryInformationThread = GetProcAddress(kernelHandle, "NtQueryInformationThread");
	if (!pNtQueryInformationThread) {
		printf("getting NtQueryInformationThread failed\n");
		return 1;
	}

	myNtQueryInformationThread = reinterpret_cast<signatureNtQueryInformationThread>(pNtQueryInformationThread);

	auto handle = OpenThread(THREAD_QUERY_INFORMATION, FALSE, info->ThreadId);
	if (!handle) {
		//printf("OpenThread failed: %d\n", GetLastError());
		return 1;
	}

	WCHAR* imgname = GetImageName(handle, info);
	if (!imgname) {
		return 1;
	}
	DisplayTime(header->Time);
	if (isThreadRemote(handle, info)) printf("CreateRemoteThread: Process %d: Thread %d created\nImage: %ws\n", info->ProcessId, info->ThreadId, imgname);
	else printf("Process %d: Thread %d created\nImage: %ws\n", info->ProcessId, info->ThreadId, imgname);

	return 0;
}

WCHAR* GetImageName(HANDLE handle, ThreadInfo* info) {

	THREAD_BASIC_INFORMATION tbi;
	ULONG len;

	auto status = myNtQueryInformationThread(handle, (THREADINFOCLASS)0, &tbi, sizeof(THREAD_BASIC_INFORMATION), &len);
	if (!NT_SUCCESS(status)) {
		printf("NtQueryInformationThread failed: %x\n", status);
		return nullptr;
	}

	handle = OpenProcess(PROCESS_VM_READ, FALSE, info->ProcessId);
	if (!handle) {
		printf("OpenProcess failed: %d\n", GetLastError());
		return nullptr;
	}
	TEB teb;
	PEB peb;
	RTL_USER_PROCESS_PARAMETERS param;
	size_t outlen;

	status = ReadProcessMemory(handle, tbi.TebBaseAddress, &teb, sizeof(TEB), &outlen);
	if (!status) {
		printf("failed ReadProcessMemory: %d\n", GetLastError());
		return nullptr;
	}

	status = ReadProcessMemory(handle, teb.ProcessEnvironmentBlock, &peb, sizeof(PEB), &outlen);
	if (!status) {
		printf("failed ReadProcessMemory: %d\n", GetLastError());
		return nullptr;
	}

	status = ReadProcessMemory(handle, peb.ProcessParameters, &param, sizeof(RTL_USER_PROCESS_PARAMETERS), &outlen);
	if (!status) {
		printf("failed ReadProcessMemory: %d\n", GetLastError());
		return nullptr;
	}
	WCHAR* imgname = (WCHAR*)malloc(param.ImagePathName.MaximumLength);
	if (imgname == nullptr || !imgname) {
		printf("failed malloc\n");
		return nullptr;
	}
	RtlZeroMemory(imgname, param.ImagePathName.MaximumLength);
	status = ReadProcessMemory(handle, param.ImagePathName.Buffer, imgname, param.ImagePathName.MaximumLength, &outlen);
	if (!status) {
		printf("failed ReadProcessMemory: %d\n", GetLastError());
		return nullptr;
	}
	return imgname;

}

BOOL isThreadRemote(HANDLE handle, ThreadInfo* info) {
	BOOL isLast;
	ULONG len;

	auto status = myNtQueryInformationThread(handle, (THREADINFOCLASS)12, &isLast, sizeof(BOOL), &len);
	if (!NT_SUCCESS(status)) {
		printf("NtQueryInformationThread failed: %x\n", status);
		return FALSE;
	}

	return isLast;

}