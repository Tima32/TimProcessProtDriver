#pragma once
//#include <ntifs.h>

#define TIM_PROCESS_PROT_DEVICE 0x8000


//защищает вызвавшего
#define IOCTL_PROTECT_ME CTL_CODE(TIM_PROCESS_PROT_DEVICE, 0x800, /*METHOD_NEITHER*/METHOD_BUFFERED, FILE_ANY_ACCESS)

//Ожидает запуск приложения и зщищает его
struct ProtectLaunch
{
	static constexpr size_t max_len = 260;
	wchar_t path[max_len];
};
#define IOCTL_PROTECT_LAUNCH CTL_CODE(TIM_PROCESS_PROT_DEVICE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

//Ожидает запуск приложенияи защищает его, также предоставляет административныеправа
struct ProtectModerator
{
	static constexpr size_t max_len = 260;
	wchar_t path[max_len];
};
#define IOCTL_PROTECT_MODERATOR CTL_CODE(TIM_PROCESS_PROT_DEVICE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

//возвращает true если вызвавшая программа находится под защитой
struct OIMIsProtected
{
	bool prot{false};
};
#define IOCTL_IM_IS_PROTECT CTL_CODE(TIM_PROCESS_PROT_DEVICE, 0x810, METHOD_BUFFERED, FILE_ANY_ACCESS)

//Возвращает true если программа с указанным id под защитой (admin)
struct IIsProtected
{
	HANDLE h_process;
};
struct OIsProtected
{
	bool is_protected{ false };
};
#define IOCTL_IS_PROTECT CTL_CODE(TIM_PROCESS_PROT_DEVICE, 0x811, METHOD_BUFFERED, FILE_ANY_ACCESS)