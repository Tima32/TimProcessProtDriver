#pragma once
#include <ntifs.h>

class Notifications
{
public:
	

	Notifications() = delete;

	static NTSTATUS Constructor(IN PDRIVER_OBJECT driver);
	static void Destructor(IN PDRIVER_OBJECT driver);

private:
	static void OnProcessNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo);

	static OB_PREOP_CALLBACK_STATUS OnPreOpenProcess(PVOID, POB_PRE_OPERATION_INFORMATION Info);
	static void OnPostOpenProcess(PVOID RegistrationContext, POB_POST_OPERATION_INFORMATION pOperationInformation);

	static OB_PREOP_CALLBACK_STATUS OnPreOpenThread(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION pOperationInformation);

	static OB_PREOP_CALLBACK_STATUS OnPreOpenFile(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION OperationInformation);

	static bool process_notify;
	static PVOID hRegistration;
};