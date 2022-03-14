#include "Mutex.hpp"

namespace TimSTD
{
	void GMutex::constructor()
	{
		KeInitializeGuardedMutex(&mutex);
	}
	void GMutex::lock()
	{
		KeAcquireGuardedMutex(&mutex);
	}
	void GMutex::unlock()
	{
		KeReleaseGuardedMutex(&mutex);
	}


	GLockGuard::GLockGuard(GMutex& m) : mutex(m)
	{
		mutex.lock();
	}
	GLockGuard::~GLockGuard()
	{
		mutex.unlock();
	}
}