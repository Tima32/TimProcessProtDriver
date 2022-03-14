#include <ntifs.h>

#include "GlobalData.hpp"
#include "DriverIO.hpp"
#include "Notifications.hpp"

VOID UnloadDriver(IN PDRIVER_OBJECT pDriver)
{
	UNREFERENCED_PARAMETER(pDriver);

	DriverIO::Destructor(pDriver);
	Notifications::Destructor(pDriver);

	GlobalData::protection_list_mutex.lock();
	GlobalData::TerminateAllProtectedProcesses();
	GlobalData::protection_list_mutex.unlock();

	GlobalData::destructor();

	KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt:UnloadDriver>Info: Unload Driver\n"));

	//LARGE_INTEGER interval = { 0 };
	//constexpr __int64 mytime = 10000 * 1000;
	//interval.QuadPart = mytime * (-10 * 1000); //1 nano
	//auto s = KeDelayExecutionThread(KernelMode, FALSE, &interval); //1 sec
	//KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt:UnloadDriver>Info: Whait End %u\n", s));
}

//void* operator new(size_t size, POOL_TYPE type, ULONG tag = 0)
//{
//	KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt:ON>Info: %016I64X %I32u %I32u\n", size, type, tag));
//
//	auto p = tag == 0 ? ExAllocatePool(type, size) : ExAllocatePoolWithTag(type, size, tag);
//	if (p == nullptr)
//		KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_ERROR_LEVEL, "<TimProcessProt:DriverEntry>Info: Bed Alloc\n"));
//	return p;
//}
//void operator delete (void* pointer, size_t size)
//{
//	UNREFERENCED_PARAMETER(size);
//
//	KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt:OD>Info: %p %I64u\n", pointer, size));
//
//	ExFreePool((pointer));
//}
//void operator delete[](void* pointer)
//{
//	KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt:OD[]>Info: %p\n", pointer));
//	if (pointer != nullptr)
//		ExFreePool((pointer));
//}


extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriver, IN PUNICODE_STRING pRegPath)
{
	UNREFERENCED_PARAMETER(pRegPath);
	pDriver->DriverUnload = UnloadDriver;

	GlobalData::constructor();
	DriverIO::Constructor(pDriver);
	Notifications::Constructor(pDriver);

	auto requesting_process = PsGetCurrentProcess();
	auto requesting_process_id = PsGetProcessId(requesting_process);
	PUNICODE_STRING requesting_image_name = nullptr;
	NTSTATUS s = SeLocateProcessImageName(requesting_process, &requesting_image_name);
	if (s == 0)
		KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt:DriverEntry>Info: ID %I64u %wZ\n", requesting_process_id, requesting_image_name));
	ExFreePoolWithTag(requesting_image_name, 0);

	KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt:DriverEntry>Info: Load Driver\n"));
	return STATUS_SUCCESS;
}