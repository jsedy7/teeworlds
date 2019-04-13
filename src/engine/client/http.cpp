#include "http.h"
#include <base/system.h>
#include <curl/curl.h>

void HttpBuffer::Release()
{
	if(m_pData)
		mem_free(m_pData);
	m_pData = 0;
	m_Cursor = 0;
	m_Size = 0;
}

bool ParseHttpUrl(const char* pUrl, char* pHostName, char* pBasePath)
{
	dbg_msg("http", "%s", curl_url());
	if(str_comp_num(pUrl, "http://", 7) != 0)
	{
		dbg_msg("http", "url does not start with 'http://' (%s)", pUrl);
		return false;
	}

	pUrl += 7;

	const char* pFirstSlash = str_find(pUrl, "/");
	if(pFirstSlash)
	{
		int HostNameLen = pFirstSlash-pUrl;
		dbg_assert(HostNameLen < 255, "HostName too long");
		mem_move(pHostName, pUrl, HostNameLen);
		pHostName[HostNameLen] = 0;

		int BasePathLen = str_length(pFirstSlash);
		dbg_assert(BasePathLen < 255, "BasePath too long");
		mem_move(pBasePath, pFirstSlash, BasePathLen);
		pBasePath[BasePathLen] = 0;
	}
	else
	{
		int HostNameLen = str_length(pUrl);
		dbg_assert(HostNameLen < 255, "HostName too long");
		mem_move(pHostName, pUrl, HostNameLen);
		pHostName[HostNameLen] = 0;

		pBasePath[0] = 0;
	}

	return true;
}

static size_t CurlWriteCallback(char *pChunkBuffer, size_t Size1, size_t ChunkSize, void *pUserData)
{
	HttpBuffer& Buffer = *(HttpBuffer*)pUserData;

	// we're out of space, double capacity
	if(Buffer.m_Cursor + ChunkSize > Buffer.m_Size)
	{
		int NewSize = Buffer.m_Size * 2;
		if(Buffer.m_Cursor + ChunkSize > NewSize)
			NewSize = (Buffer.m_Cursor + ChunkSize) * 2;

		char* pNewData = (char*)mem_alloc(NewSize, 1);
		mem_move(pNewData, Buffer.m_pData, Buffer.m_Size);
		mem_free(Buffer.m_pData);
		Buffer.m_pData = pNewData;
		Buffer.m_Size = NewSize;
	}

	mem_move(Buffer.m_pData + Buffer.m_Cursor, pChunkBuffer, ChunkSize);
	Buffer.m_Cursor += ChunkSize;

	return ChunkSize;
}

bool HttpRequestPage(const char* pUrl, HttpBuffer* pHttpBuffer)
{
	// init buffer
	dbg_assert(pHttpBuffer->m_pData == 0, "http buffer is not empty");
	pHttpBuffer->m_Cursor = 0; // 1Kb
	pHttpBuffer->m_Size = 1024; // 1Kb
	pHttpBuffer->m_pData = (char*)mem_alloc(pHttpBuffer->m_Size, 1);

	CURL *pCurl;
	CURLcode Response;

	curl_global_init(CURL_GLOBAL_DEFAULT);

	pCurl = curl_easy_init();
	if(pCurl)
	{
		curl_easy_setopt(pCurl, CURLOPT_URL, pUrl);
		curl_easy_setopt(pCurl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
		curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, pHttpBuffer);
		/* Perform the request, res will get the return code */
		Response = curl_easy_perform(pCurl);
		/* Check for errors */
		if(Response != CURLE_OK)
			dbg_msg("curl", "curl_easy_perform() failed: %s", curl_easy_strerror(Response));

		/* always cleanup */
		curl_easy_cleanup(pCurl);
	}

	curl_global_cleanup();
	return Response == CURLE_OK;
}
