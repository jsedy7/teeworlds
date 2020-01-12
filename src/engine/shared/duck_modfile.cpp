#include "duck_modfile.h"
#include <base/system.h>
#include <engine/storage.h>
#include <zlib.h>

struct CPath
{
	char m_aBuff[512];
};

struct CFileSearch
{
	IStorage* m_pStorage;
	CPath m_BaseBath;
	array<CPath>* m_paFilePathList;
};

static int ListDirCallback(const char* pName, int IsDir, int StorageType, void *pUser)
{
	CFileSearch FileSearch = *(CFileSearch*)pUser; // copy

	if(IsDir)
	{
		if(pName[0] != '.')
		{
			//dbg_msg("duck", "ListDirCallback dir='%s'", pName);
			str_append(FileSearch.m_BaseBath.m_aBuff, "/", sizeof(FileSearch.m_BaseBath.m_aBuff));
			str_append(FileSearch.m_BaseBath.m_aBuff, pName, sizeof(FileSearch.m_BaseBath.m_aBuff));
			//dbg_msg("duck", "recursing... dir='%s'", FileSearch.m_BaseBath.m_aBuff);
			FileSearch.m_pStorage->ListDirectory(StorageType, FileSearch.m_BaseBath.m_aBuff, ListDirCallback, &FileSearch);
		}
	}
	else
	{
		CPath FileStr;
		str_copy(FileStr.m_aBuff, pName, sizeof(FileStr.m_aBuff));

		CPath FilePath = FileSearch.m_BaseBath;
		str_append(FilePath.m_aBuff, "/", sizeof(FilePath.m_aBuff));
		str_append(FilePath.m_aBuff, pName, sizeof(FilePath.m_aBuff));
		FileSearch.m_paFilePathList->add(FilePath);
		//dbg_msg("duck", "ListDirCallback file='%s'", pName);
	}

	return 0;
}

bool DuckCreateModFileFromFolder(IStorage* pStorage, const char *pModPath, CDuckModFile *pOut)
{
	enum { STORAGE_TYPE_CURRENT_DIR=2 };

	dbg_assert(pStorage && pModPath && pOut, "invalid argument");

	pOut->Reset();

	// find main script file
	char aMainScriptPath[512];
	str_copy(aMainScriptPath, pModPath, sizeof(aMainScriptPath));
	str_append(aMainScriptPath, "/" MAIN_SCRIPT_FILE, sizeof(aMainScriptPath));

	IOHANDLE MainScriptFile = pStorage->OpenFile(aMainScriptPath, IOFLAG_READ, STORAGE_TYPE_CURRENT_DIR);
	if(!MainScriptFile)
	{
		dbg_msg("duck", "CompressMod: could not find %s file (%s)", MAIN_SCRIPT_FILE, aMainScriptPath);
		return false;
	}
	io_close(MainScriptFile);

	// get files recursively on disk
	array<CPath> aFilePathList;
	CFileSearch FileSearch;
	str_copy(FileSearch.m_BaseBath.m_aBuff, pModPath, sizeof(FileSearch.m_BaseBath.m_aBuff));
	FileSearch.m_paFilePathList = &aFilePathList;
	FileSearch.m_pStorage = pStorage;

	pStorage->ListDirectory(STORAGE_TYPE_CURRENT_DIR, pModPath, ListDirCallback, &FileSearch);
	const int ModPathStrLen = str_length(pModPath);

	// find valid and required files
	const char* aRequiredFiles[] = {
		MAIN_SCRIPT_FILE,
		"mod_info.json"
	};
	const int RequiredFilesCount = sizeof(aRequiredFiles)/sizeof(aRequiredFiles[0]);
	int FoundRequiredFilesCount = 0;

	array<CPath> aValidFilePathList;

	const int FileCount = aFilePathList.size();
	const CPath* pFilePaths = aFilePathList.base_ptr();
	for(int i = 0; i < FileCount; i++)
	{
		dbg_msg("duck", "file='%s'", pFilePaths[i].m_aBuff);
		if(str_endswith(pFilePaths[i].m_aBuff, SCRIPTFILE_EXT) || str_endswith(pFilePaths[i].m_aBuff, ".json") || str_endswith(pFilePaths[i].m_aBuff, ".png") || str_endswith(pFilePaths[i].m_aBuff, ".wv"))
		{
			const char* pRelPath = str_find(pFilePaths[i].m_aBuff, pModPath);
			dbg_assert(pRelPath != 0, "base mod path should be found");
			pRelPath += ModPathStrLen+1; // skip 'mod_path/'

			for(int r = 0; r < RequiredFilesCount; r++)
			{
				 // TODO: can 2 files have the same name?
				if(str_comp(pRelPath, aRequiredFiles[r]) == 0)
					FoundRequiredFilesCount++;
			}

			aValidFilePathList.add(pFilePaths[i]);
		}
	}

	if(FoundRequiredFilesCount != RequiredFilesCount)
	{
		dbg_msg("duck", "CompressMod: mod is missing a required file, required files are: ");
		for(int r = 0; r < RequiredFilesCount; r++)
		{
			dbg_msg("duck", "    - %s", aRequiredFiles[r]);
		}
		return false;
	}

	char aModRootPath[512];
	pStorage->GetCompletePath(STORAGE_TYPE_CURRENT_DIR, pModPath, aModRootPath, sizeof(aModRootPath));

	// get last folder as mod name
	const char* pModName = str_find(pModPath, "/");
	while(pModName)
	{
		pModName++;
		const char* pModNameTemp = str_find(pModName, "/");
		if(pModNameTemp)
			pModName = pModNameTemp;
		else
			break;
	}

	// simple pack of file data

	// char[4] - "DUCK"
	// # for each file
	// int - filepath string length
	// string - filepath
	// int - filesize
	// raw - filedata

	CGrowBuffer FilePackBuff;
	FilePackBuff.Grow(1024*1024); // 1Mb
	FilePackBuff.Append("DUCK", 4);

	const int ValidFileCount = aValidFilePathList.size();
	const CPath* pValidFilePaths = aValidFilePathList.base_ptr();

	for(int i = 0; i < ValidFileCount; i++)
	{
		const char* pRelPath = str_find(pValidFilePaths[i].m_aBuff, pModPath);
		dbg_assert(pRelPath != 0, "base mod path should be found");
		pRelPath += ModPathStrLen+1; // skip 'mod_path/'

		dbg_msg("duck", "valid_file='%s'", pRelPath);

		IOHANDLE File = pStorage->OpenFile(pValidFilePaths[i].m_aBuff, IOFLAG_READ, STORAGE_TYPE_CURRENT_DIR);
		if(!File)
		{
			dbg_msg("duck", "CompressMod: Error, can't open '%s'", pValidFilePaths[i].m_aBuff);
			return false;
		}

		int FileSize = (int)io_length(File);
		char *pFileData = (char *)mem_alloc(FileSize, 1);
		io_read(File, pFileData, FileSize);
		io_close(File);

		const int RelPathLen = str_length(pRelPath);
		FilePackBuff.Append(&RelPathLen, sizeof(RelPathLen));
		FilePackBuff.Append(pRelPath, RelPathLen);

		FilePackBuff.Append(&FileSize, sizeof(FileSize));
		FilePackBuff.Append(pFileData, FileSize);

		mem_free(pFileData);
	}

	CGrowBuffer* pOutFileBuffer = &pOut->m_FileBuffer;
	pOutFileBuffer->Clear();
	pOutFileBuffer->Grow(compressBound(FilePackBuff.m_Size));

	uLongf DestSize = pOutFileBuffer->m_Capacity;
	int CompRet = compress2((Bytef*)pOutFileBuffer->m_pData, &DestSize, (const Bytef*)FilePackBuff.m_pData, FilePackBuff.m_Size, Z_BEST_COMPRESSION);
	//int CompRet = compress((Bytef*)pOutFileBuffer->m_pData, &DestSize, (const Bytef*)FilePackBuff.m_pData, FilePackBuff.m_Size);

	if(CompRet != Z_OK)
	{
		switch(CompRet)
		{
			case Z_MEM_ERROR:
				dbg_msg("duck", "CompressMod: Error, not enough memory");
				break;
			case Z_BUF_ERROR:
				dbg_msg("duck", "CompressMod: Error, not enough room in the output buffer");
				break;
			case Z_STREAM_ERROR:
				dbg_msg("duck", "CompressMod: Error, level invalid");
				break;
			default:
				dbg_break(); // should never be reached
		}
		return false;
	}

	pOutFileBuffer->m_Size = DestSize;

#if CONF_DEBUG
	{
		CGrowBuffer TestBuff;
		TestBuff.Grow(FilePackBuff.m_Size);
		uLongf TestDestSize = TestBuff.m_Capacity;
		int UncompRet = uncompress((Bytef*)TestBuff.m_pData, &TestDestSize, (const Bytef*)pOutFileBuffer->m_pData, pOutFileBuffer->m_Size);
		dbg_assert(UncompRet == Z_OK && (int)TestDestSize == FilePackBuff.m_Size, "compression test failed");
	}
#endif

	pOut->m_Sha256 = sha256(pOutFileBuffer->m_pData, pOutFileBuffer->m_Size);
	char aModSha256Str[SHA256_MAXSTRSIZE];
	sha256_str(pOut->m_Sha256, aModSha256Str, sizeof(aModSha256Str));

	dbg_msg("duck", "done creating mod file pack size=%d comp_ratio=%g sha256=%s", (int)DestSize, FilePackBuff.m_Size/(double)DestSize, aModSha256Str);

	return true;
}

bool DuckExtractFilesFromModFile(const CDuckModFile *pIn, CDuckModFileExtracted *pOut, bool IsVerbose)
{
	dbg_assert(pIn && pOut, "invalid argument");

	pOut->Reset();
	pOut->m_Sha256 = pIn->m_Sha256;

	if(IsVerbose)
		dbg_msg("extract", "EXTRACTING *COMPRESSED* MOD");

	// uncompress
	const char* pCompBuff = pIn->m_FileBuffer.m_pData;
	const int CompBuffSize = pIn->m_FileBuffer.m_Size;

	CGrowBuffer& ExtractBuff = pOut->m_FileData;
	ExtractBuff.Grow(CompBuffSize * 3);

	uLongf DestSize = ExtractBuff.m_Capacity;
	int UncompRet = uncompress((Bytef*)ExtractBuff.m_pData, &DestSize, (const Bytef*)pCompBuff, CompBuffSize);
	ExtractBuff.m_Size = DestSize;

	// TODO: send extracted size instead?
	int GrowAttempts = 4;
	while(UncompRet == Z_BUF_ERROR && GrowAttempts--)
	{
		ExtractBuff.Grow(ExtractBuff.m_Capacity * 2);
		DestSize = ExtractBuff.m_Capacity;
		UncompRet = uncompress((Bytef*)ExtractBuff.m_pData, &DestSize, (const Bytef*)pCompBuff, CompBuffSize);
		ExtractBuff.m_Size = DestSize;

		if(IsVerbose)
			dbg_msg("extract", "failed to extract, growing... (%d %d)", ExtractBuff.m_Capacity, GrowAttempts);
	}

	if(UncompRet != Z_OK)
	{
		switch(UncompRet)
		{
			case Z_MEM_ERROR:
				dbg_msg("duck", "DecompressMod: Error, not enough memory");
				break;
			case Z_BUF_ERROR:
				dbg_msg("duck", "DecompressMod: Error, not enough room in the output buffer");
				break;
			case Z_DATA_ERROR:
				dbg_msg("duck", "DecompressMod: Error, data incomplete or corrupted");
				break;
			default:
				dbg_break(); // should never be reached
		}
		return false;
	}

	// read decompressed pack file

	// header
	const char* pFilePack = ExtractBuff.m_pData;
	const int FilePackSize = ExtractBuff.m_Size;
	const char* const pFilePackEnd = pFilePack + FilePackSize;

	if(str_comp_num(pFilePack, "DUCK", 4) != 0)
	{
		dbg_msg("duck", "DecompressMod: Error, invalid pack file");
		return false;
	}

	pFilePack += 4;

	// find required files
	const char* aRequiredFiles[] = {
		MAIN_SCRIPT_FILE,
		"mod_info.json"
	};
	const int RequiredFilesCount = sizeof(aRequiredFiles)/sizeof(aRequiredFiles[0]);
	int FoundRequiredFilesCount = 0;

	const char* pCursor = pFilePack;
	while(pCursor < pFilePackEnd)
	{
		const int FilePathLen = *(int*)pCursor;
		pCursor += 4;
		const char* pFilePath = pCursor;
		pCursor += FilePathLen;
		const int FileSize = *(int*)pCursor;
		pCursor += 4;
		// const char* pFileData = pCursor;
		pCursor += FileSize;

		char aFilePath[512];
		dbg_assert(FilePathLen > 0 && FilePathLen < (int)sizeof(aFilePath)-1, "FilePathLen too large");
		mem_move(aFilePath, pFilePath, FilePathLen);
		aFilePath[FilePathLen] = 0;

		if(IsVerbose)
			dbg_msg("duck", "extracted_file='%s' size=%d", aFilePath, FileSize);

		for(int r = 0; r < RequiredFilesCount; r++)
		{
			// TODO: can 2 files have the same name?
			// TODO: not very efficient, but oh well
			const char* pFind = str_find(aFilePath, aRequiredFiles[r]);
			if(pFind && FilePathLen-(pFind-aFilePath) == str_length(aRequiredFiles[r]))
				FoundRequiredFilesCount++;
		}
	}

	if(FoundRequiredFilesCount != RequiredFilesCount)
	{
		dbg_msg("duck", "mod is missing a required file, required files are: ");
		for(int r = 0; r < RequiredFilesCount; r++)
		{
			dbg_msg("duck", "    - %s", aRequiredFiles[r]);
		}
		return false;
	}

	pCursor = pFilePack;
	while(pCursor < pFilePackEnd)
	{
		const int FilePathLen = *(int*)pCursor;
		pCursor += 4;
		const char* pFilePath = pCursor;
		pCursor += FilePathLen;
		const int FileSize = *(int*)pCursor;
		pCursor += 4;
		const char* pFileData = pCursor;
		pCursor += FileSize;

		CDuckModFileExtracted::CFileEntry Entry;
		dbg_assert(FilePathLen > 0 && FilePathLen < (int)sizeof(Entry.m_aPath)-1, "FilePathLen too large");
		mem_move(Entry.m_aPath, pFilePath, FilePathLen);
		Entry.m_aPath[FilePathLen] = 0;
		Entry.m_pData = pFileData;
		Entry.m_Size = FileSize;

		// TODO: validate files
		pOut->m_Files.add(Entry);
	}

	return true;
}
