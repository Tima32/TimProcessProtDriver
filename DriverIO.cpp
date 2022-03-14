#include "DriverIO.hpp"

#include "DeviceIOCodes.hpp"
#include "GlobalData.hpp"

UNICODE_STRING dev_name = RTL_CONSTANT_STRING(L"\\Device\\TimProcessProt");
UNICODE_STRING sym_link = RTL_CONSTANT_STRING(L"\\??\\TimProcessProt");
UNICODE_STRING dcl = RTL_CONSTANT_STRING(L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;AU)");


NTSTATUS DriverIO::Constructor(IN PDRIVER_OBJECT driver)
{
	driver->MajorFunction[IRP_MJ_CREATE] = IRPOpen;
	driver->MajorFunction[IRP_MJ_CLOSE] = IRPClose;
	driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IRPDeviceControl;

	NTSTATUS status = WdmlibIoCreateDeviceSecure(driver, 0, &dev_name, FILE_DEVICE_UNKNOWN, 0, FALSE, &dcl, nullptr, &device_object);
	if (!NT_SUCCESS(status)) {
		KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_ERROR_LEVEL, "<TimProcessProt:DriverIO:Constructor>Error: Failed to create device object (0x%08X)\n", status));
		return status;
	}

	status = IoCreateSymbolicLink(&sym_link, &dev_name);
	if (!NT_SUCCESS(status)) {
		KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_ERROR_LEVEL, "<TimProcessProt:DriverIO:Constructor>Error: Failed to create symbolic link (0x%08X)\n", status));
		Destructor(driver);
		return status;
	}
	sym_link_created = true;

	return status;
}
void DriverIO::Destructor(IN PDRIVER_OBJECT driver)
{
	UNREFERENCED_PARAMETER(driver);
	if (sym_link_created)
	{
		IoDeleteSymbolicLink(&sym_link);
		sym_link_created = false;
	}
	if (device_object)
	{
		IoDeleteDevice(device_object);
		device_object = nullptr;
	}
}

NTSTATUS DriverIO::IRPOpen(_In_ PDEVICE_OBJECT p_device_object, _In_ PIRP irp)
{
	UNREFERENCED_PARAMETER(p_device_object);
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
NTSTATUS DriverIO::IRPClose(_In_ PDEVICE_OBJECT p_device_object, _In_ PIRP irp)
{
	UNREFERENCED_PARAMETER(p_device_object);
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DriverIO::IRPDeviceControl(PDEVICE_OBJECT, PIRP irp)
{
	auto stack = IoGetCurrentIrpStackLocation(irp); // IO_STACK_LOCATION*
	auto ioctl_code = stack->Parameters.DeviceIoControl.IoControlCode;
	auto status = STATUS_SUCCESS;

	if (ioctl_code == IOCTL_PROTECT_ME)
	{
		status = IRPDeviceControlPritectMe(irp);
	}
	else if (ioctl_code == IOCTL_PROTECT_LAUNCH)
	{
		status = IRPDeviceControlPritectLaunch(irp);
	}
	else if (ioctl_code == IOCTL_PROTECT_MODERATOR)
	{
		status = IRPDeviceControlPritectModerator(irp);
	}
	else if (ioctl_code == IOCTL_IM_IS_PROTECT)
	{
		status = IRPDeviceControlIMIsProtected(irp);
	}
	else if (ioctl_code == IOCTL_IS_PROTECT)
	{
		status = IRPDeviceControlIsProtected(irp);
	}
	else
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
	}

	irp->IoStatus.Status = status;
	//irp->IoStatus.Information = returnLength; //заполняет обзаботчик запроса
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}
//----------------------------------------------------------------------
NTSTATUS DriverIO::IRPDeviceControlPritectMe(PIRP irp)
{
	auto requesting_process = PsGetCurrentProcess();
	auto requesting_process_id = PsGetProcessId(requesting_process);

	NTSTATUS status = STATUS_SUCCESS;
	GlobalData::ProtectionProcess pp;

	GlobalData::protection_list_mutex.lock();

	auto find_id = GlobalData::PRLFindProcessByID(requesting_process_id);
	if (find_id != -1)//программа уже защищена
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		goto protect_me_exit;
	}
	pp.initLaunched(requesting_process_id);
	if (!GlobalData::protection_list.push_back(pp))
		status = STATUS_INVALID_DEVICE_REQUEST;

protect_me_exit:
	GlobalData::protection_list_mutex.unlock();
	irp->IoStatus.Information = 0;
	return status;
}
NTSTATUS DriverIO::IRPDeviceControlPritectLaunch(PIRP irp)
{
	auto requesting_process = PsGetCurrentProcess();
	auto requesting_process_id = PsGetProcessId(requesting_process);

	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(irp);
	ULONG inLength = irpsp->Parameters.DeviceIoControl.InputBufferLength;

	if (sizeof(ProtectModerator) != inLength)
		return STATUS_INVALID_PARAMETER;//неверный размер

	UNICODE_STRING process_path;
	process_path.Buffer = (wchar_t*)irp->AssociatedIrp.SystemBuffer;
	process_path.Buffer[ProtectModerator::max_len - 1] = L'\0';
	process_path.MaximumLength = ProtectModerator::max_len * sizeof(wchar_t);
	process_path.Length = USHORT(wcslen(process_path.Buffer) * sizeof(wchar_t));
	KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_ERROR_LEVEL, "<TimProcessProt:DriverIO:IRPDeviceControlPritectLaunch>Info: Launch: %wZ\n", process_path));


	NTSTATUS status = STATUS_SUCCESS;
	GlobalData::ProtectionProcess pp;
	if (!pp.initNotLaunched(requesting_process_id, &process_path))
		return STATUS_INVALID_DEVICE_REQUEST;

	GlobalData::protection_list_mutex.lock();
	if (!GlobalData::protection_list.push_back(pp))
		status = STATUS_INVALID_DEVICE_REQUEST;
	GlobalData::protection_list_mutex.unlock();

	irp->IoStatus.Information = 0;
	return status;
}
NTSTATUS DriverIO::IRPDeviceControlPritectModerator(PIRP irp)
{
	auto requesting_process = PsGetCurrentProcess();
	auto requesting_process_id = PsGetProcessId(requesting_process);

	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(irp);
	ULONG inLength = irpsp->Parameters.DeviceIoControl.InputBufferLength;

	if (sizeof(ProtectModerator) != inLength)
		return STATUS_INVALID_PARAMETER;//неверный размер

	UNICODE_STRING process_path;
	process_path.Buffer = (wchar_t*)irp->AssociatedIrp.SystemBuffer;
	process_path.Buffer[ProtectModerator::max_len - 1] = L'\0';
	process_path.MaximumLength = ProtectModerator::max_len * sizeof(wchar_t);
	process_path.Length = USHORT(wcslen(process_path.Buffer) * sizeof(wchar_t));
	KdPrintEx((DPFLTR_ACPI_ID, DPFLTR_ERROR_LEVEL, "<TimProcessProt:DriverIO:IRPDeviceControlPritectModerator>Info: Moderator: %wZ\n", process_path));


	NTSTATUS status = STATUS_SUCCESS;
	GlobalData::ProtectionProcess pp;
	if (!pp.initNotLaunchedModerator(requesting_process_id, &process_path))
		return STATUS_INVALID_DEVICE_REQUEST;//Не удалось инициализировать структуру

	GlobalData::protection_list_mutex.lock();
	if (!GlobalData::protection_list.push_back(pp))
		status = STATUS_INVALID_DEVICE_REQUEST;
	GlobalData::protection_list_mutex.unlock();

	irp->IoStatus.Information = 0;
	return status;
}
//----------------------------------------------------------------------
NTSTATUS DriverIO::IRPDeviceControlIMIsProtected(PIRP irp)
{
	auto requesting_process = PsGetCurrentProcess();
	auto requesting_process_id = PsGetProcessId(requesting_process);

	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(irp);
	ULONG out_length_buff = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG out_length = 0;

	NTSTATUS status = STATUS_SUCCESS;
	if (out_length_buff < sizeof(OIMIsProtected))
	{
		//выходной буфер слишком мал
		status = STATUS_INVALID_DEVICE_REQUEST;
		goto end_reguest;
	}

	GlobalData::protection_list_mutex.lock();
	{
		OIMIsProtected* is_protected = (OIMIsProtected*)irp->AssociatedIrp.SystemBuffer;
		auto find_id = GlobalData::PRLFindProcessByID(requesting_process_id);
		is_protected->prot = find_id != -1;
		out_length = sizeof(OIMIsProtected);
	}
	GlobalData::protection_list_mutex.unlock();

	end_reguest:
	irp->IoStatus.Information = out_length;
	return status;
}
NTSTATUS DriverIO::IRPDeviceControlIsProtected(PIRP irp)
{
	auto requesting_process = PsGetCurrentProcess();
	auto requesting_process_id = PsGetProcessId(requesting_process);

	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(irp);
	void* in_buff = irp->AssociatedIrp.SystemBuffer;
	ULONG in_buff_length = irpsp->Parameters.DeviceIoControl.InputBufferLength;
	void* out_buff = irp->AssociatedIrp.SystemBuffer;
	ULONG out_buff_length = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG out_length = 0;

	NTSTATUS status = STATUS_SUCCESS;
	if (in_buff_length < sizeof(IIsProtected))
	{
		//входной буфер слишком мал
		status = STATUS_INVALID_DEVICE_REQUEST;
		goto end_reguest;
	}
	if (out_buff_length < sizeof(OIsProtected))
	{
		//выходной буфер слишком мал
		status = STATUS_INVALID_DEVICE_REQUEST;
		goto end_reguest;
	}

	GlobalData::protection_list_mutex.lock();
	{
		IIsProtected* reguest_in = (IIsProtected*)in_buff;

		//есть ли у запрашивающего повышеные прова?
		auto reguest_protected_process = GlobalData::PRLFindProcessByID(requesting_process_id);
		if (reguest_protected_process == -1)
		{
			//нет прав
			status = STATUS_ACCESS_DENIED;
			goto end_request_logic;
		}

		HANDLE target_id = reguest_in->h_process;
		OIsProtected* reguest_out = (OIsProtected*)out_buff;
		reguest_out->is_protected = false;
		out_length = sizeof(OIsProtected);

		auto target_process = GlobalData::PRLFindProcessByID(target_id);
		if (target_process != -1)
			reguest_out->is_protected = true;
	}
	end_request_logic:
	GlobalData::protection_list_mutex.unlock();

end_reguest:
	irp->IoStatus.Information = out_length;
	return status;
}


PDEVICE_OBJECT DriverIO::device_object{nullptr};
bool DriverIO::sym_link_created{false};