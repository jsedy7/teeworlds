#pragma once
#include "growbuffer.h"
#include <base/hash.h>
#include <base/tl/array.h>

#ifdef DUCK_LUA_BACKEND
#define MAIN_SCRIPT_FILE "main.lua"
#define SCRIPTFILE_EXT ".lua"
#endif


struct CDuckModFile
{
	SHA256_DIGEST m_Sha256;
	CGrowBuffer m_FileBuffer;

	inline void Reset()
	{
		m_Sha256 = SHA256_ZEROED;
		m_FileBuffer.Clear();
	}
};

struct CDuckModFileExtracted
{
	struct CFileEntry
	{
		char m_aPath[512];
		const char* m_pData;
		int m_Size;
	};

	array<CFileEntry> m_Files;
	SHA256_DIGEST m_Sha256;
	CGrowBuffer m_FileData;

	inline void Reset()
	{
		m_Sha256 = SHA256_ZEROED;
		m_FileData.Clear();
		m_Files.clear();
	}
};

struct IStorage;

bool DuckCreateModFileFromFolder(IStorage *pStorage, const char* pModPath, CDuckModFile* pOut);
bool DuckExtractFilesFromModFile(const CDuckModFile* pIn, CDuckModFileExtracted* pOut, bool IsVerbose = false);
