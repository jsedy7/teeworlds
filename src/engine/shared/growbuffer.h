#pragma once
#include <base/system.h>
#include <base/math.h>

//#define DBG_MEM_USAGE

#ifdef DBG_MEM_USAGE
#define Grow(Size) GrowImp(Size, __FILE__, __LINE__)
#define Append(Buff, Size) AppendImp(Buff, Size, __FILE__, __LINE__)
#endif

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

#ifdef DBG_MEM_USAGE
	inline void GrowImp(int NewCapacity, const char* file, int line)
#else
	inline void Grow(int NewCapacity)
#endif
	{
		if(NewCapacity < m_Capacity)
			return;

		char* pNewData = (char*)mem_alloc(NewCapacity, 1);
		dbg_assert(pNewData != 0, "failed to grow buffer, could not allocate");
		mem_move(pNewData, m_pData, m_Size);
		mem_free(m_pData);

#ifdef DBG_MEM_USAGE
		dbg_msg("memory", "Grow(%s:%d) %llx : %d -> %llx : %d", file, line, m_pData, m_Capacity, pNewData, NewCapacity);
#endif

		m_pData = pNewData;
		m_Capacity = NewCapacity;
	}

#ifdef DBG_MEM_USAGE
	inline char* AppendImp(const void* pBuff, int BuffSize, const char* file, int line)
#else
	inline char* Append(const void* pBuff, int BuffSize)
#endif
	{
		// grow
		if(BuffSize + m_Size > m_Capacity)
		{
			int NewCapacity = max(m_Capacity * 2, BuffSize + m_Size);

#ifdef DBG_MEM_USAGE
			GrowImp(NewCapacity, file, line);
#else
			Grow(NewCapacity);
#endif
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
