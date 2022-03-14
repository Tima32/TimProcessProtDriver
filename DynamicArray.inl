#pragma once

namespace TimSTD
{
	template <typename T>
	bool DynamicArray<T>::constructor(POOL_TYPE p_t, ULONG t, size_t d_l)
	{
		destructor();

		arr = nullptr;
		len = d_l;
		element_count = 0;

		pool_type = p_t;
		tag = t;

		if (len == 0)
			return false;

		arr = (T*)ExAllocatePoolWithTag(pool_type, sizeof(T)* len, tag);
		if (!arr)
			return false;
		return true;
	}
	template <typename T>
	void DynamicArray<T>::destructor()
	{
		if (arr)
			ExFreePoolWithTag(arr, tag);
	}

	template <typename T>
	bool DynamicArray<T>::push_back(const T& elem)
	{
		if (element_count == len)
		{
			size_t new_len = len * 2;
			T* new_mem = (T*)ExAllocatePoolWithTag(pool_type, sizeof(T) * new_len, tag);
			if (!new_mem)
				return false;
			memcpy(new_mem, arr, sizeof(T) * element_count);
			ExFreePoolWithTag(arr, tag);
			arr = new_mem;
			len = new_len;
		}

		arr[element_count] = elem;
		++element_count;
		return true;
	}
	template <typename T>
	void DynamicArray<T>::erase(size_t num)
	{
		for (size_t i = num; i < element_count - 1; ++i)
		{
			arr[i] = arr[i + 1];
		}
		--element_count;
	}

	template <typename T>
	size_t DynamicArray<T>::size() const
	{
		return element_count;
	}

	template <typename T>
	T& DynamicArray<T>::operator[](size_t elem)
	{
		return arr[elem];
	}
}