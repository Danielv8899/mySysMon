#pragma once

//define control codes that are valid for both driver and user code
#define IOCTL_START_RECORDING CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_STOP_RECORDING CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_NEITHER, FILE_ANY_ACCESS)

enum class ItemType : short {
	None,
	ProcessCreate,
	ProcessExit,
	ThreadCreate,
	ThreadDelete,
};

struct ItemHeader {
	ItemType Type;
	USHORT Size;
	LARGE_INTEGER Time;
};

struct ProcessExitInfo : ItemHeader {
	ULONG ProcessId;
};

template<typename T>
struct LItem {
	LIST_ENTRY ListEntry;
	T Item;
};

struct ProcessCreateInfo : ItemHeader {
	ULONG ProcessId;
	ULONG ParentProcessId;
	USHORT CommandLineLength;
	USHORT CommandLineOffset;
	USHORT ImageNameLength;
	USHORT ImageNameOffset;
};

struct ThreadInfo : ItemHeader {
	ULONG ProcessId;
	ULONG ThreadId;
};