#include "ioctl.h"
#include "notify.h"

NTSTATUS StartRecordingRoutine()
{
    auto status = STATUS_SUCCESS;
    status = PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)NotifyRoutine, FALSE);
    if (!NT_SUCCESS(status))
    {
		KdPrint(("StartRecordingRoutine: PsSetCreateProcessNotifyRoutineEx failed: %x\n", status));
	}
    status = PsSetCreateThreadNotifyRoutineEx(PsCreateThreadNotifySubsystems, ThreadRoutine);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("StartRecordingRoutine: PsSetCreateThreadNotifyRoutineEx failed: %x\n", status));
    }
    return status;
}

NTSTATUS StopRecordingRoutine()
{
    auto status = STATUS_SUCCESS;
    status = PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)NotifyRoutine, TRUE);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("StopRecordingRoutine: PsSetCreateProcessNotifyRoutineEx failed: %x\n", status));
    }

    status = PsRemoveCreateThreadNotifyRoutine((PCREATE_THREAD_NOTIFY_ROUTINE)ThreadRoutine);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("StartRecordingRoutine: PsSetCreateThreadNotifyRoutineEx failed: %x\n", status));
    }
    return status;
}
