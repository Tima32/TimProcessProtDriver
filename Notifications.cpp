#include "Notifications.hpp"

#include "GlobalData.hpp"

//#define PROCESS_TERMINATE         (0x0001) //winnt
//#define PROCESS_CREATE_THREAD     (0x0002) //winnt
//#define PROCESS_SET_SESSIONID     (0x0004) //winnt
//#define PROCESS_VM_OPERATION      (0x0008) //winnt
//#define PROCESS_VM_READ           (0x0010) //winnt
//#define PROCESS_VM_WRITE          (0x0020) //winnt
////begin_ntddk begin_wdm begin_ntifs
//#define PROCESS_DUP_HANDLE        (0x0040) //winnt
////end_ntddk end_wdm end_ntifs
//#define PROCESS_CREATE_PROCESS    (0x0080) //winnt
//#define PROCESS_SET_QUOTA         (0x0100) //winnt
//#define PROCESS_SET_INFORMATION   (0x0200) //winnt
//#define PROCESS_QUERY_INFORMATION (0x0400) //winnt
//#define PROCESS_SET_PORT          (0x0800)
//#define PROCESS_SUSPEND_RESUME    (0x0800) //winnt

//IoIs32bitProcess программа 32bit?

typedef struct _LDR_DATA_TABLE_ENTRY64
{
	LIST_ENTRY64	InLoadOrderLinks;
	LIST_ENTRY64	InMemoryOrderLinks;
	LIST_ENTRY64	InInitializationOrderLinks;
	PVOID			DllBase;
	PVOID			EntryPoint;
	ULONG			SizeOfImage;
	UNICODE_STRING	FullDllName;
	UNICODE_STRING 	BaseDllName;
	ULONG			Flags;
	USHORT			LoadCount;
	USHORT			TlsIndex;
	PVOID			SectionPointer;
	ULONG			CheckSum;
	PVOID			LoadedImports;
	PVOID			EntryPointActivationContext;
	PVOID			PatchInformation;
	LIST_ENTRY64	ForwarderLinks;
	LIST_ENTRY64	ServiceTagLinks;
	LIST_ENTRY64	StaticLinks;
	PVOID			ContextInformation;
	ULONG64			OriginalBase;
	LARGE_INTEGER	LoadTime;
} LDR_DATA_TABLE_ENTRY64, * PLDR_DATA_TABLE_ENTRY64;
typedef struct _OBJECT_TYPE_INITIALIZER
{
	UINT16       Length;
	union
	{
		UINT8        ObjectTypeFlags;
		struct DATA
		{
			UINT8        CaseInsensitive : 1;
			UINT8        UnnamedObjectsOnly : 1;
			UINT8        UseDefaultObject : 1;
			UINT8        SecurityRequired : 1;
			UINT8        MaintainHandleCount : 1;
			UINT8        MaintainTypeList : 1;
			UINT8        SupportsObjectCallbacks : 1;
		};
		DATA data;
	};
	ULONG32      ObjectTypeCode;
	ULONG32      InvalidAttributes;
	struct _GENERIC_MAPPING GenericMapping;
	ULONG32      ValidAccessMask;
	ULONG32      RetainAccess;
	enum _POOL_TYPE PoolType;
	ULONG32      DefaultPagedPoolCharge;
	ULONG32      DefaultNonPagedPoolCharge;
	PVOID        DumpProcedure;
	PVOID        OpenProcedure;
	PVOID		 CloseProcedure;
	PVOID		 DeleteProcedure;
	PVOID		 ParseProcedure;
	PVOID        SecurityProcedure;
	PVOID		 QueryNameProcedure;
	PVOID		 OkayToCloseProcedure;
}OBJECT_TYPE_INITIALIZER, * POBJECT_TYPE_INITIALIZER;
typedef struct _OBJECT_TYPE_TEMP
{
	struct _LIST_ENTRY TypeList;
	struct _UNICODE_STRING Name;
	VOID* DefaultObject;
	UINT8        Index;
	UINT8        _PADDING0_[0x3];
	ULONG32      TotalNumberOfObjects;
	ULONG32      TotalNumberOfHandles;
	ULONG32      HighWaterNumberOfObjects;
	ULONG32      HighWaterNumberOfHandles;
	UINT8        _PADDING1_[0x4];
	struct _OBJECT_TYPE_INITIALIZER TypeInfo;
	ULONG64 TypeLock;
	ULONG32      Key;
	UINT8        _PADDING2_[0x4];
	struct _LIST_ENTRY CallbackList;
}OBJECT_TYPE_TEMP, * POBJECT_TYPE_TEMP;
VOID EnableObType(POBJECT_TYPE ObjectType)
{
	POBJECT_TYPE_TEMP  ObjectTypeTemp = (POBJECT_TYPE_TEMP)ObjectType;
	ObjectTypeTemp->TypeInfo.data.SupportsObjectCallbacks = 1;
}
NTSTATUS Notifications::Constructor(IN PDRIVER_OBJECT driver)
{
	UNREFERENCED_PARAMETER(driver);

	NTSTATUS status = PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, false);
	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_ERROR_LEVEL, "<TimProcessProt:Notifications:Constructor>Error: Failed to registry notify (0x%08X)\n", status));
		return status;
	}
	process_notify = true;


	OB_CALLBACK_REGISTRATION obRegistration = { 0, };
	OB_OPERATION_REGISTRATION opRegistration[3] = { 0, };

	obRegistration.Version = ObGetFilterVersion();	// Get version
	obRegistration.OperationRegistrationCount = 2;	// OB_OPERATION_REGISTRATION count, opRegistration[3] количество перехватчмков 
	RtlInitUnicodeString(&obRegistration.Altitude, L"300000");	// Altitude - приоритет очереди
	obRegistration.RegistrationContext = NULL;

	opRegistration[0].ObjectType = PsProcessType;
	opRegistration[0].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;	// Callback создания handle процесса
	opRegistration[0].PreOperation = OnPreOpenProcess;	// PreOperation
	opRegistration[0].PostOperation = OnPostOpenProcess;	// PostOperation

	opRegistration[1].ObjectType = PsThreadType;
	opRegistration[1].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;	// Callback созлания handle потока
	opRegistration[1].PreOperation = OnPreOpenThread;	// PreOperation
	opRegistration[1].PostOperation = NULL;	// PostOperation

	//EnableObType(*IoFileObjectType);//разрешить перехват handle файлов
	opRegistration[2].ObjectType = IoFileObjectType;
	opRegistration[2].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;	// Callback создания handle файла
	opRegistration[2].PreOperation = OnPreOpenFile;	// PreOperation
	opRegistration[2].PostOperation = NULL;	// PostOperation

	obRegistration.OperationRegistration = (OB_OPERATION_REGISTRATION*)&opRegistration;	// Добавление ссылки на массив операций

	status = ObRegisterCallbacks(&obRegistration, &hRegistration);
	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_ERROR_LEVEL, "<TimProcessProt:Notifications:Constructor>Error: Failed to registry ObRegisterCallbacks (0x%08X)\n", status));
		Destructor(driver);
		return status;
	}

	return STATUS_SUCCESS;
}
void Notifications::Destructor(IN PDRIVER_OBJECT driver)
{
	UNREFERENCED_PARAMETER(driver);

	if (process_notify)
	{
		PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, true);
		process_notify = false;
	}

	if (hRegistration)	// ДЭ№й µо·ПїЎ ЅЗЖРЗТ °жїм ї№їЬ Гіё®
	{
		ObUnRegisterCallbacks(hRegistration);
		hRegistration = nullptr;
	}

	//protection_list.~DynamicArray();
}

//private:
void OpenFile(PUNICODE_STRING fn)
{
	OBJECT_ATTRIBUTES obj_attributeR;
	HANDLE filehandleR = nullptr;
	IO_STATUS_BLOCK IostatusblockR = { 0 };

	InitializeObjectAttributes(&obj_attributeR, fn, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

	NTSTATUS status = ZwCreateFile(&filehandleR, GENERIC_READ, &obj_attributeR, &IostatusblockR, NULL, FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ, FILE_OPEN, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);

	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt>Info: OpenErr\n"));
		return;
	}
	else
	{
		KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt>Info: OpenSuccess\n"));
	}

	char fileinfo[sizeof(FILE_NAME_INFORMATION) + 256] = { 0 };
	FILE_NAME_INFORMATION* fi = (FILE_NAME_INFORMATION*)fileinfo;
	status = ZwQueryInformationFile(filehandleR, &IostatusblockR, fileinfo, sizeof(FILE_NAME_INFORMATION) + 256, FileNameInformation);
	if (NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt> File size is %ls\n", fi->FileName));
	}
	else
		KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt> File size check faild %lu\n", status));

	if(filehandleR != nullptr)
		ZwClose(filehandleR);
}
void Notifications::OnProcessNotify(PEPROCESS Process, /*ID*/ HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	UNREFERENCED_PARAMETER(Process);
	UNREFERENCED_PARAMETER(ProcessId);

	if (CreateInfo) {
		PUNICODE_STRING requesting_image_name = nullptr;
		SeLocateProcessImageName(Process, &requesting_image_name);

		KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt:Notifications:OnProcessNotify>Info: Process created %wZ\n", /*CreateInfo->ImageFileName*/requesting_image_name));
		//OpenFile(requesting_image_name);
		
		GlobalData::protection_list_mutex.lock();
		KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt>Test: size: %I64u\n", GlobalData::protection_list.size()));
		for (size_t i = 0; i < GlobalData::protection_list.size(); ++i)
		{
			auto& pp = GlobalData::protection_list[i];
			if (pp.process_id != nullptr)
				continue;

			if (pp.parent_id != CreateInfo->ParentProcessId)
				continue;
			KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt:Notifications:OnProcessNotify>Parent: id matches\n"));

			if (wcscmp(pp.process_path.Buffer, requesting_image_name->Buffer) != 0)
				continue;
			KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt:Notifications:OnProcessNotify>Yiiii\n"));
			pp.process_id = ProcessId;
		}
		GlobalData::protection_list_mutex.unlock();

		ExFreePoolWithTag(requesting_image_name, 0);
	}
	else {
		GlobalData::protection_list_mutex.lock();

		//Если завершился зщищаемый процесс, то происходит удаление из списка
		size_t pr_num = GlobalData::PRLFindProcessByID(ProcessId);
		if (pr_num != -1)
		{
			GlobalData::PRLDeleteElement(pr_num);
			KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt:Notifications:OnProcessNotify>Info: Delited\n"));
		}

		//Работа с родителями процессов
		for (size_t i = 0; i < GlobalData::protection_list.size(); ++i)
		{
			auto& pp = GlobalData::protection_list[i];

			//удаление не полученных процессов при завершении родителя
			if (pp.parent_id == ProcessId && pp.process_id == nullptr)
			{
				GlobalData::PRLDeleteElement(i);
				KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt:Notifications:OnProcessNotify>Info: Waiting removed.\n"));
				--i;//Компенсирование сдвига после удаления
			}
			//Зануление родителя
			else if (pp.parent_id == ProcessId)
			{
				pp.parent_id = nullptr;
			}
		}
		GlobalData::protection_list_mutex.unlock();
		KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_INFO_LEVEL, "<TimProcessProt:Notifications:OnProcessNotify>Info: The process is complete.\n"));
	}

}

OB_PREOP_CALLBACK_STATUS Notifications::OnPreOpenProcess(PVOID, POB_PRE_OPERATION_INFORMATION Info)
{
	if (Info->KernelHandle)
		return OB_PREOP_SUCCESS;
	//отсеивание не интересующих операций
	if (Info->Operation != OB_OPERATION_HANDLE_CREATE &&
		Info->Operation != OB_OPERATION_HANDLE_DUPLICATE)
		return OB_PREOP_SUCCESS;

	auto target_process = (PEPROCESS)Info->Object;
	auto target_process_id = PsGetProcessId(target_process);

	auto requesting_process = PsGetCurrentProcess();
	auto requesting_process_id = PsGetProcessId(requesting_process);

	GlobalData::protection_list_mutex.lock();
	const size_t target_pr_num = GlobalData::PRLFindProcessByID(target_process_id);
	const size_t reguest_pr_num = GlobalData::PRLFindProcessByID(requesting_process_id);
	bool reguest_pr_is_moderator = false;

	//Проверка на повышенные права (Eng: Checking for elevated rights)
	if (reguest_pr_num != -1)
		reguest_pr_is_moderator = GlobalData::protection_list[reguest_pr_num].moderator;

	//Ограничиваем права если запрашивающий не родитель и не модераиор
	if (target_pr_num != -1 &&
		GlobalData::protection_list[target_pr_num].parent_id != requesting_process_id &&
		reguest_pr_is_moderator == false)
	{
		PUNICODE_STRING requesting_image_name = nullptr;
		SeLocateProcessImageName(requesting_process, &requesting_image_name);
		
		if (!wcsstr(requesting_image_name->Buffer, L"csrss.exe") /*&& !wcsstr(requesting_image_name->Buffer, L"svchost.exe")*/)
		{
			KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_ERROR_LEVEL,
				"<TimProcessProt:Notifications:OnPreOpenProcess>Info: Access denied %wZ\n",
				requesting_image_name));
			Info->Parameters->CreateHandleInformation.DesiredAccess &= GlobalData::protection_list[target_pr_num].process_access_mask;
		}

		ExFreePoolWithTag(requesting_image_name, 0);
	}
	GlobalData::protection_list_mutex.unlock();


	return OB_PREOP_SUCCESS;

}
void Notifications::OnPostOpenProcess(PVOID /*RegistrationContext*/, POB_POST_OPERATION_INFORMATION /*pOperationInformation*/)
{

}

OB_PREOP_CALLBACK_STATUS Notifications::OnPreOpenThread(PVOID , POB_PRE_OPERATION_INFORMATION Info)
{
	if (Info->KernelHandle)
		return OB_PREOP_SUCCESS;

	//отсеивание не интересующих операций
	if (Info->Operation != OB_OPERATION_HANDLE_CREATE &&
		Info->Operation != OB_OPERATION_HANDLE_DUPLICATE)
		return OB_PREOP_SUCCESS;

	auto target_process = (PETHREAD)Info->Object;
	auto target_process_id = PsGetThreadProcessId(target_process);

	auto requesting_process = PsGetCurrentProcess();
	auto requesting_process_id = PsGetProcessId(requesting_process);

	GlobalData::protection_list_mutex.lock();
	size_t target_pr_num = GlobalData::PRLFindProcessByID(target_process_id);
	const size_t reguest_pr_num = GlobalData::PRLFindProcessByID(requesting_process_id);
	bool reguest_pr_is_moderator = false;

	//Проверка на повышенные права (Eng: Checking for elevated rights)
	if (reguest_pr_num != -1)
		reguest_pr_is_moderator = GlobalData::protection_list[reguest_pr_num].moderator;

	//Ограничиваем права если запрашивающий не родитель и не модераиор
	if (target_pr_num != -1 &&
		GlobalData::protection_list[target_pr_num].parent_id != requesting_process_id &&
		reguest_pr_is_moderator == false)
	{
		PUNICODE_STRING requesting_image_name = nullptr;
		SeLocateProcessImageName(requesting_process, &requesting_image_name);

		if (!wcsstr(requesting_image_name->Buffer, L"csrss.exe") /*&& !wcsstr(requesting_image_name->Buffer, L"svchost.exe")*/)
		{
			KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_ERROR_LEVEL,
				"<TimProcessProt:Notifications:OnPreOpenThread>Info: Access denied %wZ\n",
				requesting_image_name));
			Info->Parameters->CreateHandleInformation.DesiredAccess &= GlobalData::protection_list[target_pr_num].thread_access_mask;
		}

		ExFreePoolWithTag(requesting_image_name, 0);
	}
	GlobalData::protection_list_mutex.unlock();


	return OB_PREOP_SUCCESS;
}

UNICODE_STRING  GetFilePathByFileObject(PVOID FileObject)
{
	POBJECT_NAME_INFORMATION ObjetNameInfor;
	if (NT_SUCCESS(IoQueryFileDosDeviceName((PFILE_OBJECT)FileObject, &ObjetNameInfor)))
	{
		return ObjetNameInfor->Name;
	}
	return UNICODE_STRING(0);
}
OB_PREOP_CALLBACK_STATUS Notifications::OnPreOpenFile(PVOID /*RegistrationContext*/, POB_PRE_OPERATION_INFORMATION OperationInformation)
{
	UNICODE_STRING uniDosName;
	UNICODE_STRING uniFilePath;
	PFILE_OBJECT FileObject = (PFILE_OBJECT)OperationInformation->Object;
	HANDLE CurrentProcessId = PsGetCurrentProcessId();

	if (OperationInformation->ObjectType != *IoFileObjectType)
		return OB_PREOP_SUCCESS;

	//Отфильтровать недопустимые указатели (Eng: Filter out invalid pointers)
	if (FileObject->FileName.Buffer == NULL ||
		!MmIsAddressValid(FileObject->FileName.Buffer) ||
		FileObject->DeviceObject == NULL ||
		!MmIsAddressValid(FileObject->DeviceObject))
	{
		return OB_PREOP_SUCCESS;
	}

	uniFilePath = GetFilePathByFileObject(FileObject);

	if (uniFilePath.Buffer == NULL || uniFilePath.Length == 0)
	{
		return OB_PREOP_SUCCESS;
	}

	if (wcsstr(uniFilePath.Buffer, L"C:\\blocking_test.txt"))
	{
		if (FileObject->DeleteAccess == TRUE || FileObject->WriteAccess == FALSE)
		{
			if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE)
				OperationInformation->Parameters->CreateHandleInformation.DesiredAccess = 0;
			if (OperationInformation->Operation == OB_OPERATION_HANDLE_DUPLICATE)
				OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess = 0;
		}
	}
	RtlVolumeDeviceToDosName(FileObject->DeviceObject, &uniDosName);
	DbgPrint("PID : %I64u File : %wZ  %wZ\r\n", ULONG64(CurrentProcessId), &uniDosName, &uniFilePath);
	return OB_PREOP_SUCCESS;
}

bool Notifications::process_notify = false;
PVOID Notifications::hRegistration = nullptr;