#pragma once
#include <ntifs.h>
#include <Wdmsec.h>

class DriverIO
{
public:
	DriverIO() = delete;

	static NTSTATUS Constructor(IN PDRIVER_OBJECT driver);
	static void Destructor(IN PDRIVER_OBJECT driver);

private:
	static NTSTATUS IRPOpen(_In_ PDEVICE_OBJECT device_object, _In_ PIRP irp);
	static NTSTATUS IRPClose(_In_ PDEVICE_OBJECT device_object, _In_ PIRP irp);

	static NTSTATUS IRPDeviceControl(PDEVICE_OBJECT, PIRP irp);
	//---------------------------------------------------------
	static NTSTATUS IRPDeviceControlPritectMe(PIRP irp);
	static NTSTATUS IRPDeviceControlPritectLaunch(PIRP irp);
	static NTSTATUS IRPDeviceControlPritectModerator(PIRP irp);
	//---------------------------------------------------------
	static NTSTATUS IRPDeviceControlIMIsProtected(PIRP irp);
	static NTSTATUS IRPDeviceControlIsProtected(PIRP irp);//(admin)

	static PDEVICE_OBJECT device_object;
	static bool sym_link_created;
};