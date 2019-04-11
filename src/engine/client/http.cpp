#include "http.h"
#include <base/system.h>
#include <curl/curl.h>

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

static size_t CurlWriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	return nmemb;
}

bool HttpRequestPage(const char* pUrl)
{
	CURL *pCurl;
	CURLcode Response;

	curl_global_init(CURL_GLOBAL_DEFAULT);

	pCurl = curl_easy_init();
	if(pCurl)
	{
		curl_easy_setopt(pCurl, CURLOPT_URL, "http://github.com/LordSk/teeworlds/archive/3.2.zip");
		curl_easy_setopt(pCurl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
		curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, NULL);
		/* Perform the request, res will get the return code */
		Response = curl_easy_perform(pCurl);
		/* Check for errors */
		if(Response != CURLE_OK)
			dbg_msg("curl", "curl_easy_perform() failed: %s", curl_easy_strerror(Response));

		/* always cleanup */
		curl_easy_cleanup(pCurl);
	}

	curl_global_cleanup();
	return true;
}
