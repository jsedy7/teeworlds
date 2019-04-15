#include "duktape_comp.h"
#include <engine/storage.h>
#include <generated/protocol.h>
#include <stdint.h>

#include <engine/client/http.h>
#include <zip.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t i32;

// TODO: move
bool ExtractAndInstallMod(const HttpBuffer* pHttpZipData, IStorage* pStorage)
{
	dbg_msg("unzip", "EXTRACTING AND INSTALLING MOD");

	char aUserMoodsPath[512];
	pStorage->GetCompletePath(IStorage::TYPE_SAVE, "mods", aUserMoodsPath, sizeof(aUserMoodsPath));
	fs_makedir(aUserMoodsPath); // DUCK

	// TODO: reduce hash string length?
	SHA256_DIGEST Sha256 = sha256(pHttpZipData->m_pData, pHttpZipData->m_Size);
	char aSha256Str[SHA256_MAXSTRSIZE];
	sha256_str(Sha256, aSha256Str, sizeof(aSha256Str));

	char aModRootPath[512];
	str_copy(aModRootPath, aUserMoodsPath, sizeof(aModRootPath));
	str_append(aModRootPath, "/", sizeof(aModRootPath));
	str_append(aModRootPath, aSha256Str, sizeof(aModRootPath));

	// FIXME: remove this
	str_append(aUserMoodsPath, "/temp.zip", sizeof(aUserMoodsPath));
	IOHANDLE File = io_open(aUserMoodsPath, IOFLAG_WRITE);
	io_write(File, pHttpZipData->m_pData, pHttpZipData->m_Size);
	io_close(File);

	dbg_msg("unzip", "OPENING %s", aUserMoodsPath);

	int Error = 0;
	zip *pZipArchive = zip_open(aUserMoodsPath, 0, &Error);
	if(pZipArchive == NULL)
	{
		char aErrorBuff[512];
		zip_error_to_str(aErrorBuff, sizeof(aErrorBuff), Error, errno);
		dbg_msg("unzip", "Error opening '%s' [%s]", aUserMoodsPath, aErrorBuff);
		return false;
	}

	dbg_msg("unzip", "CREATE directory '%s'", aModRootPath);
	if(fs_makedir(aModRootPath) != 0)
	{
		dbg_msg("unzip", "Failed to create directory '%s'", aModRootPath);
		return false;
	}

	const int EntryCount = zip_get_num_entries(pZipArchive, 0);
	for(int i = 0; i < EntryCount; i++)
	{
		zip_stat_t EntryStat;
		if(zip_stat_index(pZipArchive, i, 0, &EntryStat) != 0)
			continue;

		dbg_msg("unzip", "Name: %s, Size: %llu, mtime: [%u]", EntryStat.name, EntryStat.size, (unsigned int)EntryStat.mtime);

		const int NameLen = str_length(EntryStat.name);
		if(EntryStat.name[NameLen-1] == '/')
		{
			// create sub directory
			char aSubFolder[512];
			str_copy(aSubFolder, aModRootPath, sizeof(aSubFolder));
			str_append(aSubFolder, "/", sizeof(aSubFolder));
			str_append(aSubFolder, EntryStat.name, sizeof(aSubFolder));

			dbg_msg("unzip", "CREATE SUB directory '%s'", aSubFolder);
			if(fs_makedir(aSubFolder) != 0)
			{
				dbg_msg("unzip", "Failed to create directory '%s'", aSubFolder);
				return false;
			}
		}
		else
		{
			zip_file_t* pFileZip = zip_fopen_index(pZipArchive, i, 0);
			if(!pFileZip)
			{
				dbg_msg("unzip", "Error reading file '%s'", EntryStat.name);
				return false;
			}

			// create file on disk
			char aFilePath[256];
			str_copy(aFilePath, aModRootPath, sizeof(aFilePath));
			str_append(aFilePath, "/", sizeof(aFilePath));
			str_append(aFilePath, EntryStat.name, sizeof(aFilePath));

			IOHANDLE FileExtracted = io_open(aFilePath, IOFLAG_WRITE);
			if(!FileExtracted)
			{
				dbg_msg("unzip", "Error creating file '%s'", aFilePath);
				return false;
			}

			// read zip file data and write to file on disk
			char aReadBuff[1024];
			int ReadCurrentSize = 0;
			while(ReadCurrentSize != EntryStat.size)
			{
				const int ReadLen = zip_fread(pFileZip, aReadBuff, sizeof(aReadBuff));
				if(ReadLen < 0)
				{
					dbg_msg("unzip", "Error reading file '%s'", EntryStat.name);
					return false;
				}
				io_write(FileExtracted, aReadBuff, ReadLen);
				ReadCurrentSize += ReadLen;
			}

			io_close(FileExtracted);
			zip_fclose(pFileZip);
		}
	}

	zip_close(pZipArchive);

#if 0
	unzFile ZipFile = unzOpen64(aPath);
	unz_global_info GlobalInfo;
	int r = unzGetGlobalInfo(ZipFile, &GlobalInfo);
	if(r != UNZ_OK)
	{
		dbg_msg("unzip", "could not read file global info (%d)", r);
		unzClose(ZipFile);
		dbg_break();
		return false;
	}

	for(int i = 0; i < GlobalInfo.number_entry; i++)
	{
		// Get info about current file.
		unz_file_info file_info;
		char filename[256];
		if(unzGetCurrentFileInfo(ZipFile, &file_info, filename, sizeof(filename), NULL, 0, NULL, 0) != UNZ_OK)
		{
			dbg_msg("unzip", "could not read file info");
			unzClose(ZipFile);
			return false;
		}

		dbg_msg("unzip", "FILE_ENTRY %s", filename);

		/*// Check if this entry is a directory or file.
		const size_t filename_length = str_length(filename);
		if(filename[ filename_length-1 ] == '/')
		{
			// Entry is a directory, so create it.
			printf("dir:%s\n", filename);
			//mkdir(filename);
		}
		else
		{
			// Entry is a file, so extract it.
			printf("file:%s\n", filename);
			if(unzOpenCurrentFile(ZipFile) != UNZ_OK)
			{
				dbg_msg("unzip", "could not open file");
				unzClose(ZipFile);
				return false;
			}

			// Open a file to write out the data.
			FILE *out = fopen(filename, "wb");
			if(out == NULL)
			{
				dbg_msg("unzip", "could not open destination file");
				unzCloseCurrentFile(ZipFile);
				unzClose(ZipFile);
				return false;
			}

			int error = UNZ_OK;
			do
			{
			error = unzReadCurrentFile(zipfile, read_buffer, READ_SIZE);
			if(error < 0)
			{
			printf("error %d\n", error);
			unzCloseCurrentFile(zipfile);
			unzClose(zipfile);
			return -1;
			}

			// Write data to file.
			if(error > 0)
			{
			fwrite(read_buffer, error, 1, out); // You should check return of fwrite...
			}
			} while (error > 0);

			fclose(out);
		}*/

		unzCloseCurrentFile(ZipFile);

		// Go the the next entry listed in the zip file.
		if((i+1) < GlobalInfo.number_entry)
		{
			if(unzGoToNextFile(ZipFile) != UNZ_OK)
			{
				dbg_msg("unzip", "cound not read next file");
				unzClose(ZipFile);
				return false;
			}
		}
	}

	unzClose(ZipFile);
#endif

	return true;
}

// TODO: rename?
static CDuktape* s_This = 0;
inline CDuktape* This() { return s_This; }

static duk_ret_t NativePrint(duk_context *ctx)
{
	dbg_msg("duk", "%s", duk_to_string(ctx, 0));
	return 0;  /* no return value (= undefined) */
}

duk_ret_t CDuktape::NativeRenderQuad(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 4, "Wrong argument count");
	double x = duk_to_number(ctx, 0);
	double y = duk_to_number(ctx, 1);
	double Width = duk_to_number(ctx, 2);
	double Height = duk_to_number(ctx, 3);

	IGraphics::CQuadItem Quad(x, y, Width, Height);
	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::QUAD;
	Cmd.m_Quad = Quad;
	This()->m_aRenderCmdList[This()->m_CurrentDrawSpace].add(Cmd);

	return 0;
}

duk_ret_t CDuktape::NativeRenderSetColorU32(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");
	int x = duk_to_int(ctx, 0);

	float Color[4];
	Color[0] = (x & 0xFF) / 255.f;
	Color[1] = ((x >> 8) & 0xFF) / 255.f;
	Color[2] = ((x >> 16) & 0xFF) / 255.f;
	Color[3] = ((x >> 24) & 0xFF) / 255.f;

	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::COLOR;
	memmove(Cmd.m_Color, Color, sizeof(Color));
	This()->m_aRenderCmdList[This()->m_CurrentDrawSpace].add(Cmd);

	return 0;
}

duk_ret_t CDuktape::NativeRenderSetColorF4(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 4, "Wrong argument count");

	float Color[4];
	Color[0] = duk_to_number(ctx, 0);
	Color[1] = duk_to_number(ctx, 1);
	Color[2] = duk_to_number(ctx, 2);
	Color[3] = duk_to_number(ctx, 3);

	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::COLOR;
	memmove(Cmd.m_Color, Color, sizeof(Color));
	This()->m_aRenderCmdList[This()->m_CurrentDrawSpace].add(Cmd);

	return 0;
}

duk_ret_t CDuktape::NativeSetDrawSpace(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	int ds = duk_to_int(ctx, 0);
	dbg_assert(ds >= 0 && ds < DrawSpace::_COUNT, "Draw space undefined");
	This()->m_CurrentDrawSpace = ds;

	return 0;
}

duk_ret_t CDuktape::NativeMapSetTileCollisionFlags(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 3, "Wrong argument count");

	// TODO: bound check
	int Tx = duk_to_int(ctx, 0);
	int Ty = duk_to_int(ctx, 1);
	int Flags = duk_to_int(ctx, 2);

	This()->Collision()->SetTileCollisionFlags(Tx, Ty, Flags);

	return 0;
}

template<typename IntT>
duk_ret_t CDuktape::NativeUnpackInteger(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	// get cursor, raw buffer
	duk_get_prop_string(ctx, 0, "cursor");
	int Cursor = (int)duk_to_int(ctx, -1);
	duk_pop(ctx);

	duk_get_prop_string(ctx, 0, "raw");
	duk_size_t RawBufferSize;
	const u8* pRawBuffer = (u8*)duk_get_buffer(ctx, -1, &RawBufferSize);
	duk_pop(ctx);

	/*dbg_msg("duk", "netObj.cursor = %d", Cursor);
	dbg_msg("duk", "netObj.raw = %llx", pRawBuffer);
	dbg_msg("duk", "netObj.raw.length = %d", RawBufferSize);*/

	dbg_assert(Cursor + sizeof(IntT) <= RawBufferSize, "unpack: buffer overflow");

	IntT OutInt;
	mem_copy(&OutInt, pRawBuffer + Cursor, sizeof(IntT));
	Cursor += sizeof(IntT);

	// set new cursor
	duk_push_int(ctx, Cursor);
	duk_put_prop_string(ctx, 0, "cursor");

	// return int
	duk_push_int(ctx, (int)OutInt);
	return 1;
}

duk_ret_t CDuktape::NativeUnpackFloat(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	// get cursor, raw buffer
	duk_get_prop_string(ctx, 0, "cursor");
	int Cursor = (int)duk_to_int(ctx, -1);
	duk_pop(ctx);

	duk_get_prop_string(ctx, 0, "raw");
	duk_size_t RawBufferSize;
	const u8* pRawBuffer = (u8*)duk_get_buffer(ctx, -1, &RawBufferSize);
	duk_pop(ctx);

	dbg_assert(Cursor + sizeof(float) <= RawBufferSize, "unpack: buffer overflow");

	float OutFloat;
	mem_copy(&OutFloat, pRawBuffer + Cursor, sizeof(float));
	Cursor += sizeof(float);

	// set new cursor
	duk_push_int(ctx, Cursor);
	duk_put_prop_string(ctx, 0, "cursor");

	// return float
	duk_push_number(ctx, OutFloat);
	return 1;
}

void CDuktape::PushObject()
{
	m_CurrentPushedObjID = duk_push_object(Duk());
}

void CDuktape::ObjectSetMemberInt(const char* MemberName, int Value)
{
	duk_push_int(Duk(), Value);
	duk_put_prop_string(Duk(), m_CurrentPushedObjID, MemberName);
}

void CDuktape::ObjectSetMemberFloat(const char* MemberName, float Value)
{
	duk_push_number(Duk(), Value);
	duk_put_prop_string(Duk(), m_CurrentPushedObjID, MemberName);
}

void CDuktape::ObjectSetMemberRawBuffer(const char* MemberName, const void* pRawBuffer, int RawBufferSize)
{
	duk_push_string(Duk(), MemberName);
	duk_push_fixed_buffer(Duk(), RawBufferSize);

	duk_size_t OutBufferSize;
	u8* OutBuffer = (u8*)duk_require_buffer_data(Duk(), -1, &OutBufferSize);
	dbg_assert(OutBufferSize == RawBufferSize, "buffer size differs");
	mem_copy(OutBuffer, pRawBuffer, RawBufferSize);

	int rc = duk_put_prop(Duk(), m_CurrentPushedObjID);
	dbg_assert(rc == 1, "could not put raw buffer prop");
}

CDuktape::CDuktape()
{
	s_This = this;
}

void CDuktape::OnInit()
{
	// load ducktape, eval main.js
	m_pDukContext = duk_create_heap_default();

	char aModPath[256];
	str_format(aModPath, sizeof(aModPath), "mods/duck/main.js");
	IOHANDLE ModFile = Storage()->OpenFile(aModPath, IOFLAG_READ, IStorage::TYPE_ALL);
	dbg_assert(ModFile != 0, "Could not load duck/main.js");

	int FileSize = (int)io_length(ModFile);
	char *pFileData = (char *)mem_alloc(FileSize + 1, 1);
	io_read(ModFile, pFileData, FileSize);
	pFileData[FileSize] = 0;
	io_close(ModFile);

	// function binding
	duk_push_c_function(Duk(), NativePrint, 1 /*nargs*/);
	duk_put_global_string(Duk(), "print");

	duk_push_c_function(Duk(), NativeRenderQuad, 4 /*nargs*/);
	duk_put_global_string(Duk(), "TwRenderQuad");

	duk_push_c_function(Duk(), NativeRenderSetColorU32, 1);
	duk_put_global_string(Duk(), "TwRenderSetColorU32");

	duk_push_c_function(Duk(), NativeRenderSetColorF4, 4);
	duk_put_global_string(Duk(), "TwRenderSetColorF4");

	duk_push_c_function(Duk(), NativeSetDrawSpace, 1);
	duk_put_global_string(Duk(), "TwSetDrawSpace");

	duk_push_c_function(Duk(), NativeMapSetTileCollisionFlags, 3);
	duk_put_global_string(Duk(), "TwMapSetTileCollisionFlags");

	duk_push_c_function(Duk(), NativeUnpackInteger<i32>, 1);
	duk_put_global_string(Duk(), "TwUnpackInt32");

	duk_push_c_function(Duk(), NativeUnpackInteger<u8>, 1);
	duk_put_global_string(Duk(), "TwUnpackUint8");

	duk_push_c_function(Duk(), NativeUnpackInteger<u16>, 1);
	duk_put_global_string(Duk(), "TwUnpackUint16");

	duk_push_c_function(Duk(), NativeUnpackInteger<u32>, 1);
	duk_put_global_string(Duk(), "TwUnpackUint32");

	duk_push_c_function(Duk(), NativeUnpackFloat, 1);
	duk_put_global_string(Duk(), "TwUnpackFloat");

	// Teeworlds global object
	duk_eval_string(Duk(),
		"var Teeworlds = {"
		"	DRAW_SPACE_GAME: 0,"
		"	aClients: [],"
		"};");

	// eval script
	duk_push_string(Duk(), pFileData);
	if(duk_peval(Duk()) != 0)
	{
		/* Use duk_safe_to_string() to convert error into string.  This API
		 * call is guaranteed not to throw an error during the coercion.
		 */
		dbg_msg("duk", "Script error: %s", duk_safe_to_string(Duk(), -1));
		dbg_break();
	}
	duk_pop(Duk());

	dbg_msg("duk", "main.js loaded (%d)", FileSize);
	mem_free(pFileData);

	m_CurrentDrawSpace = DrawSpace::GAME;


	// TODO: remove, testing
	HttpBuffer Buff;
	HttpRequestPage("https://github.com/LordSk/teeworlds/releases/download/1.1/gametypes.zip", &Buff);
	bool IsUnzipped = ExtractAndInstallMod(&Buff, Storage());
	dbg_assert(IsUnzipped, "rip in peace");
	Buff.Release();
}

void CDuktape::OnShutdown()
{
	duk_destroy_heap(m_pDukContext);
}

void CDuktape::OnRender()
{
	// Update Teeworlds global object
	char aEvalBuff[256];
	str_format(aEvalBuff, sizeof(aEvalBuff), "Teeworlds.intraTick = %g;", Client()->IntraGameTick());
	duk_eval_string(Duk(), aEvalBuff);
	str_format(aEvalBuff, sizeof(aEvalBuff), "Teeworlds.mapSize = { x: %d, y: %d };", Collision()->GetWidth(), Collision()->GetHeight());
	duk_eval_string(Duk(), aEvalBuff);

	// Call OnUpdate()
	duk_get_global_string(Duk(), "OnUpdate");
	/* push arguments here */
	duk_push_number(Duk(), Client()->LocalTime());
	int NumArgs = 1;
	if(duk_pcall(Duk(), NumArgs) != 0)
	{
		dbg_msg("duk", "OnUpdate(): Script error: %s", duk_safe_to_string(Duk(), -1));
		dbg_break();
	}
	duk_pop(Duk());
}

void CDuktape::OnMessage(int Msg, void* pRawMsg)
{
	if(Msg == NETMSG_DUCK_NETOBJ)
	{
		CUnpacker* pUnpacker = (CUnpacker*)pRawMsg;
		const int ObjID = pUnpacker->GetInt();
		const int ObjSize = pUnpacker->GetInt();
		const u8* pObjRawData = (u8*)pUnpacker->GetRaw(ObjSize);
		dbg_msg("duk", "DUK packed netobj, id=0x%x size=%d", ObjID, ObjSize);

		duk_get_global_string(Duk(), "OnMessage");

		// make netObj
		PushObject();
		ObjectSetMemberInt("netID", ObjID);
		ObjectSetMemberInt("cursor", 0);
		ObjectSetMemberRawBuffer("raw", pObjRawData, ObjSize);

		// call OnMessage(netObj)
		int NumArgs = 1;
		if(duk_pcall(Duk(), NumArgs) != 0)
		{
			dbg_msg("duk", "OnMessage(): Script error: %s", duk_safe_to_string(Duk(), -1));
			dbg_break();
		}
		duk_pop(Duk());
	}
}

void CDuktape::RenderDrawSpaceGame()
{
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();

	const int CmdCount = m_aRenderCmdList[DrawSpace::GAME].size();
	const CRenderCmd* aCmds = m_aRenderCmdList[DrawSpace::GAME].base_ptr();

	for(int i = 0; i < CmdCount; i++)
	{
		switch(aCmds[i].m_Type)
		{
			case CRenderCmd::COLOR: {
				const float* pColor = aCmds[i].m_Color;
				Graphics()->SetColor(pColor[0] * pColor[3], pColor[1] * pColor[3], pColor[2] * pColor[3], pColor[3]);
			} break;
			case CRenderCmd::QUAD:
				Graphics()->QuadsDrawTL(&aCmds[i].m_Quad, 1);
				break;
			default:
				dbg_assert(0, "Render command type not handled");
		}
	}

	Graphics()->QuadsEnd();

	m_aRenderCmdList[DrawSpace::GAME].clear();
}
