#pragma once
#include <ntifs.h>

namespace TimSTD
{
	class GMutex
	{
	public:
		void constructor();

		void lock();
		void unlock();

	private:
		KGUARDED_MUTEX mutex;
	};

	class GLockGuard
	{
	public:
		GLockGuard(GMutex& m);
		~GLockGuard();
	private:
		GMutex& mutex;
	};
}