#pragma once
#include <base/system.h>

// Simple growable buffer
// tries to free itself on destruct, but can be freed before
struct CGrowBuffer
{
	char* m_pData;
	int m_Size;
	int m_Capacity;

	CGrowBuffer()
	{
		m_pData = 0;
		m_Size = 0;
		m_Capacity = 0;
	}

	~CGrowBuffer()
	{
		Release();
	}

	inline void Grow(int NewCapacity)
	{
		if(NewCapacity < m_Capacity)
			return;

		char* pNewData = (char*)mem_alloc(NewCapacity, 1);
		mem_move(pNewData, m_pData, m_Size);
		mem_free(m_pData);

		m_pData = pNewData;
		m_Capacity = NewCapacity;
	}

	inline char* Append(const void* pBuff, int BuffSize)
	{
		// grow
		if(BuffSize + m_Size > m_Capacity)
		{
			int NewCapacity = m_Capacity * 2;
			if(BuffSize + m_Size > NewCapacity)
				NewCapacity = BuffSize + m_Size;

			Grow(NewCapacity);
		}

		mem_move(m_pData + m_Size, pBuff, BuffSize);
		m_Size += BuffSize;
		return m_pData + m_Size - BuffSize;
	}

	inline void Release()
	{
		if(m_pData)
			mem_free(m_pData);
		m_pData = 0;
		m_Size = 0;
		m_Capacity = 0;
	}

	inline void Clear()
	{
		m_Size = 0;
	}
};
