#pragma once
#include <ntifs.h>

#include "DynamicArray.hpp"
#include "Mutex.hpp"

class GlobalData
{
public:

	struct ProtectionProcess
	{
		enum ProcessAccessFlags : ACCESS_MASK // 1 - allow, 0 - ban
		{
			TERMINATE =         0x0001, //winnt
			CREATE_THREAD =     0x0002, //winnt
			SET_SESSIONID =     0x0004, //winnt
			VM_OPERATION =      0x0008, //winnt
			VM_READ =           0x0010, //winnt
			VM_WRITE =          0x0020, //winnt
			DUP_HANDLE =        0x0040, //winnt
			CREATE_PROCESS =    0x0080, //winnt
			SET_QUOTA =         0x0100, //winnt
			SET_INFORMATION =   0x0200, //winnt
			QUERY_INFORMATION = 0x0400, //winnt
			SUSPEND_RESUME =    0x0800, //winnt

			DEFAULT_PROCESS = TERMINATE,
			DEFAULT_THREAD = 0
		};

		HANDLE process_id{ 0 }; //0 - еще не запущен
		UNICODE_STRING process_path{ 0, 0, nullptr };

		HANDLE parent_id{ 0 };  //0 - нет или закрыт

		bool launch_protection{ false };//Защита с запуска

		ACCESS_MASK process_access_mask{ ProcessAccessFlags::DEFAULT_PROCESS };
		ACCESS_MASK thread_access_mask{ ProcessAccessFlags::DEFAULT_THREAD };

		bool ban_undercooked_code{ true };
		//list sign

		bool moderator{ false }; //модератор

		//Варианты инициализации защиты процесса
		void initLaunched(HANDLE process_id);//уже запущенный процесс
		bool initNotLaunched(HANDLE parent_id, const PUNICODE_STRING process_path);//Не запущ.
		bool initNotLaunchedModerator(HANDLE parent_id, const PUNICODE_STRING process_path);//Не запущ. с повышеными провами

		void destructor();
	};

	static bool constructor();
	static void destructor();


	//перед вызовом этих функций необходимо захватить мютекс
	static size_t PRLFindProcessByID(HANDLE id); //-1 - not found
	static void PRLDeleteElement(size_t num);//Удалить
	static void TerminateAllProtectedProcesses();//Звыершить все процессы

	//Список защищенных процессов
	static TimSTD::DynamicArray<ProtectionProcess> protection_list;//массив
	static TimSTD::GMutex protection_list_mutex;//Мютекс !(нужно блокировать перед доступом к массиву)

private:
	GlobalData();
};