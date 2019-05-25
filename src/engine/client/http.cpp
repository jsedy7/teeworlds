#include "http.h"
#include <base/system.h>
// NOTE: this defines IStorage, so this needs to be in its own cpp unit/file
#include <curl/curl.h>

static size_t CurlWriteCallback(char *pChunkBuffer, size_t Size1, size_t ChunkSize, void *pUserData)
{
	CGrowBuffer& Buffer = *(CGrowBuffer*)pUserData;
	Buffer.Append(pChunkBuffer, ChunkSize);
	return ChunkSize;
}

bool HttpRequestPage(const char* pUrl, CGrowBuffer *pHttpBuffer)
{
	// init buffer
	dbg_assert(pHttpBuffer->m_pData == 0, "http buffer is not empty");
	pHttpBuffer->m_Size = 0;
	pHttpBuffer->Grow(1024); // 1KB

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
