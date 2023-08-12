#pragma once
#include <ntddk.h>

//RAII for all synchronization objects because why not

struct MySpinLock
{
	void init();
	void lock();
	void unlock();

	private:
		KSPIN_LOCK _spinLock;
		KIRQL _oldIrql;
};

struct MyResource
{
	void init();
	void acquireExclusive();
	void releaseExclusive();
	void acquireShared();
	void releaseShared();

	private:
		ERESOURCE _resource;
};

struct MyEvent
{
	void init(BOOLEAN manualReset = FALSE, BOOLEAN initialState = FALSE);
	void set();
	void reset();
	void wait();

	private:
		KEVENT _event;
};

struct MyFastMutex
{
	void init();
	void lock();
	void unlock();

private:
	FAST_MUTEX _mutex;
};

struct MyMutex
{
	void init();
	void lock();
	void unlock();

private:
	KMUTEX _mutex;
};

struct GuardedMutex
{
	void init();
	void lock();
	void unlock();

private:
	KGUARDED_MUTEX _mutex;
};

struct MySemaphore
{
	void init(LONG count = 0, LONG limit = MAXLONG);
	void wait();
	void release();

	private:
		KSEMAPHORE _semaphore;
};

template <typename T>
struct AutoSemaphore
{
	AutoSemaphore(T& semaphore) :_semaphore(semaphore)
	{
		_semaphore.wait();
	}
	~AutoSemaphore()
	{
		_semaphore.release();
	}

	private:
		T& _semaphore;
};

template <typename T>
struct AutoLock
{
	AutoLock(T& mutex) :_mutex(mutex)
	{
		_mutex.lock();
	}
	~AutoLock()
	{
		_mutex.unlock();
	}

private:
T& _mutex;
};

template <typename T>
struct AutoEvent
{
	AutoEvent(T& event) :_event(event)
	{
		_event.wait();
	}
	~AutoEvent()
	{
		_event.set();
	}

	private:
		T& _event;
};

template <typename T>
struct AutoResource
{
	AutoResource(T& resource) :_resource(resource)
	{
		_resource.acquireExclusive();
	}
	~AutoResource()
	{
		_resource.releaseExclusive();
	}

	private:
		T& _resource;
};