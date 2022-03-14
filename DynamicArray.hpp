#pragma once
#include <ntifs.h>

namespace TimSTD
{
	template <typename T>
	class DynamicArray //не вызывает конструкторы и деструкторы
	{
	public:
		bool constructor(POOL_TYPE pool_type, ULONG tag = 0, size_t default_len = 1);
		void destructor();

		bool push_back(const T& elem);
		void erase(size_t num);

		size_t size() const;

		T& operator[](size_t elem);


	private:
		T* arr;
		size_t element_count;
		size_t len;

		POOL_TYPE pool_type;
		ULONG tag;
	};
}

#include "DynamicArray.inl"