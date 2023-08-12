#include "sync.h"


void MyFastMutex::init()
{
	ExInitializeFastMutex(&_mutex);
}
void MyFastMutex::lock()
{
	ExAcquireFastMutex(&_mutex);
}
void MyFastMutex::unlock()
{
	ExReleaseFastMutex(&_mutex);
}

void MyMutex::init()
{
	KeInitializeMutex(&_mutex, 0);
}
void MyMutex::lock()
{
	KeWaitForSingleObject(&_mutex, Executive, KernelMode, FALSE, NULL);
}
void MyMutex::unlock()
{
	KeReleaseMutex(&_mutex, FALSE);
}

void GuardedMutex::init()
{
	KeInitializeGuardedMutex(&_mutex);
}
void GuardedMutex::lock()
{
	KeAcquireGuardedMutex(&_mutex);
}
void GuardedMutex::unlock()
{
	KeReleaseGuardedMutex(&_mutex);
}

void MySemaphore::init(LONG count, LONG limit)
{
	KeInitializeSemaphore(&_semaphore, count, limit);
}

void MySemaphore::wait()
{
	KeWaitForSingleObject(&_semaphore, Executive, KernelMode, FALSE, NULL);
}

void MySemaphore::release()
{
	KeReleaseSemaphore(&_semaphore, IO_NO_INCREMENT, 1, FALSE);
}

void MyEvent::init(BOOLEAN manualReset, BOOLEAN initialState)
{
	KeInitializeEvent(&_event, manualReset ? NotificationEvent : SynchronizationEvent, initialState);
}

void MyEvent::set()
{
	KeSetEvent(&_event, IO_NO_INCREMENT, FALSE);
}

void MyEvent::reset()
{
	KeResetEvent(&_event);
}

void MyEvent::wait()
{
	KeWaitForSingleObject(&_event, Executive, KernelMode, FALSE, NULL);
}

void MyResource::init()
{
	ExInitializeResourceLite(&_resource);
}

void MyResource::acquireExclusive()
{
	ExAcquireResourceExclusiveLite(&_resource, TRUE);
}

void MyResource::releaseExclusive()
{
	ExReleaseResourceLite(&_resource);
}

void MyResource::acquireShared()
{
	ExAcquireResourceSharedLite(&_resource, TRUE);
}

void MyResource::releaseShared()
{
	ExReleaseResourceLite(&_resource);
}

void MySpinLock::init()
{
	KeInitializeSpinLock(&_spinLock);
}

void MySpinLock::lock()
{
	KeAcquireSpinLock(&_spinLock, &_oldIrql);
}

void MySpinLock::unlock()
{
	KeReleaseSpinLock(&_spinLock, _oldIrql);
}
