#include "GlobalData.hpp"

//ProtectionProcess{

void GlobalData::ProtectionProcess::initLaunched(HANDLE proc_id)
{
	process_id = proc_id;
	ban_undercooked_code = false;
}
bool GlobalData::ProtectionProcess::initNotLaunched(HANDLE p_id, const PUNICODE_STRING pr_path)
{
	//process_id = reinterpret_cast<HANDLE>(-1);

	process_path.Buffer = (wchar_t*)ExAllocatePoolWithTag(PagedPool, pr_path->MaximumLength, 0);
	if (!process_path.Buffer)
		return false;
	process_path.MaximumLength = pr_path->MaximumLength;
	process_path.Length = 0;
	RtlCopyUnicodeString(&process_path, pr_path);

	parent_id = p_id;
	launch_protection = true;

	return true;
}
bool GlobalData::ProtectionProcess::initNotLaunchedModerator(HANDLE p_id, const PUNICODE_STRING pr_path)
{
	process_id = reinterpret_cast<HANDLE>(0);

	process_path.Buffer = (wchar_t*)ExAllocatePoolWithTag(PagedPool, pr_path->MaximumLength, 0);
	if (!process_path.Buffer)
		return false;
	process_path.MaximumLength = pr_path->MaximumLength;
	process_path.Length = 0;
	RtlCopyUnicodeString(&process_path, pr_path);

	parent_id = p_id;
	launch_protection = true;//защита запуска
	moderator = true;        //права
	return true;
}

void GlobalData::ProtectionProcess::destructor()
{
	if (process_path.Buffer)
		ExFreePoolWithTag(process_path.Buffer, 0);
}

//ProtectionProcess}

bool GlobalData::constructor()
{
	if (!protection_list.constructor(PagedPool))
		goto constructor_fail;
	protection_list_mutex.constructor();

	return true;

constructor_fail:
	KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt:GlobalData:Constructor>Error: Init fail\n"));
	destructor();
	return false;
}
void GlobalData::destructor()
{
	//list
	for (size_t i = 0; i < protection_list.size(); ++i)
		protection_list[i].destructor();//вызов деструктора содержимого
	protection_list.destructor();//вызов деструктора массива

	KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt:GlobalData:Destructorr>Info: Destructed\n"));
}

size_t GlobalData::PRLFindProcessByID(HANDLE id)
{
	for (size_t i = 0; i < protection_list.size(); ++i)
	{
		if (protection_list[i].process_id == id)
		{
			return i;
		}
	}

	return size_t(-1);
}
void GlobalData::PRLDeleteElement(size_t num)
{
	protection_list[num].destructor();
	protection_list.erase(num);
}
void GlobalData::TerminateAllProtectedProcesses()
{
	for (size_t i = 0; i < protection_list.size(); ++i)
	{
		if (protection_list[i].process_id == 0)
			continue;

		HANDLE prh = nullptr;
		OBJECT_ATTRIBUTES oab = {0};
		InitializeObjectAttributes(&oab, NULL, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
		CLIENT_ID clientId = {0};
		clientId.UniqueProcess = protection_list[i].process_id;

		NTSTATUS status = ZwOpenProcess(&prh, ProtectionProcess::ProcessAccessFlags::TERMINATE, &oab, &clientId);
		if (!NT_SUCCESS(status))
		{
			KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt>Info: OpenProcess error\n"));
			continue;
		}
		KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt>Info: OpenProcess success\n"));

		status = ZwTerminateProcess(prh, STATUS_ACCESS_DENIED);
		if (!NT_SUCCESS(status))
		{
			KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt>Info: TerminateProcess error\n"));
			continue;
		}
		KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt>Info: TerminateProcess success\n"));

		ZwClose(prh);
	}
}
TimSTD::DynamicArray<GlobalData::ProtectionProcess> GlobalData::protection_list;
TimSTD::GMutex GlobalData::protection_list_mutex;