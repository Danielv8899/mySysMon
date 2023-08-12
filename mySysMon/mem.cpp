#include "mem.h"
#pragma warning(disable:4996)

void* __cdecl operator new(size_t size,POOL_TYPE poolType)
{
    return ExAllocatePoolWithTag(poolType,size,DRIVER_TAG);
}

void* malloc(size_t size, POOL_TYPE poolType)
{
	return ExAllocatePoolWithTag(poolType, size, DRIVER_TAG);
}

void __cdecl operator delete(void* p, UINT64 tak) noexcept
{
    UNREFERENCED_PARAMETER(tak);
    ExFreePool(p);
}

void* __cdecl operator new[](size_t size, POOL_TYPE poolType)
{
    return ExAllocatePoolWithTag(poolType, size, DRIVER_TAG);
}

void __cdecl operator delete[](void* p)
{
    ExFreePool(p);
}

template <typename T = void>
struct kunique_ptr {

    kunique_ptr(T* p = nullptr) :_p(p) {}

    kunique_ptr(const kunique_ptr&) = delete;
    kunique_ptr& operator=(const kunique_ptr&) = delete;

    kunique_ptr& operator=(kunique_ptr&& other)
    {
        if (this != &other)
        {
            delete _p;
            _p = other._p;
            other._p = nullptr;
        }
        return *this;
    }

    ~kunique_ptr() { delete _p; }

private:
    T* _p;
};