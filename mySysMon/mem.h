#pragma once
#include <ntddk.h>

#define DRIVER_TAG 'ybbd'
//new and delete operator overloading
void* __cdecl operator new(size_t size, POOL_TYPE poolType);
void* malloc(size_t size, POOL_TYPE poolType);
void __cdecl operator delete(void* p);
void* __cdecl operator new[](size_t size, POOL_TYPE poolType);
void __cdecl operator delete[](void* p);

