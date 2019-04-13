#pragma once

struct HttpBuffer
{
	char* m_pData;
	int m_Cursor;
	int m_Size;

	HttpBuffer()
	{
		m_pData = 0;
		m_Cursor = 0;
		m_Size = 0;
	}

	void Release();
};

bool HttpRequestPage(const char* pUrl, HttpBuffer* pHttpBuffer);
