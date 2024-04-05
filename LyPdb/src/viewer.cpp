#include "viewer.hpp"
#include <ntimage.h>
#include <ksocket/sockets.h>
#include <ntstrsafe.h>

namespace oxygenPdb
{
	EXTERN_C int sprintf_s(
		char* buffer,
		size_t sizeOfBuffer,
		const char* format,
		...);

	EXTERN_C int swprintf(wchar_t* String,
		size_t Count, 
		const wchar_t* Format, ...);

	const char* downUrl = "http://msdl.blackint3.com/download/symbols/";

	struct PdbInfo
	{
		ULONG Signature;
		GUID Guid;
		ULONG Age;
		char PdbFileName[1];
	};

	using ptr_t = UINT_PTR;
	using pdbInfo_t = std::pair<kstd::string, kstd::string>;

	pdbInfo_t PdbViewer::getPdbInfo(ptr_t mBase)
	{
		const auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(mBase);
		const auto ntHeader = reinterpret_cast<IMAGE_NT_HEADERS*>(mBase + dosHeader->e_lfanew);
		const auto dbgDir = reinterpret_cast<IMAGE_DEBUG_DIRECTORY*>(mBase + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress);
		const auto pdbInfo= reinterpret_cast<PdbInfo*>(mBase + dbgDir->AddressOfRawData);

		// 获取 pdb guid

		kstd::string pdbGuid(40, 0);
		sprintf_s(pdbGuid.data(), pdbGuid.size(), "%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x%x",
			pdbInfo->Guid.Data1, pdbInfo->Guid.Data2, pdbInfo->Guid.Data3,
			pdbInfo->Guid.Data4[0], pdbInfo->Guid.Data4[1], pdbInfo->Guid.Data4[2], pdbInfo->Guid.Data4[3],
			pdbInfo->Guid.Data4[4], pdbInfo->Guid.Data4[5], pdbInfo->Guid.Data4[6], pdbInfo->Guid.Data4[7],
			pdbInfo->Age);
		pdbGuid.resize(strlen(pdbGuid.data()));
		
		// 获取 pdb name

		kstd::string pdbName(pdbInfo->PdbFileName);
		
		return std::pair(pdbGuid,pdbName);
	}

	NTSTATUS CreateDirectory(UNICODE_STRING Path)
	{
		NTSTATUS Status = 0;
		HANDLE h_File = 0;
		OBJECT_ATTRIBUTES Oa = { 0 };
		IO_STATUS_BLOCK Io = { 0 };

		InitializeObjectAttributes(&Oa, &Path, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, 0);

		Status = ZwCreateFile(
			&h_File,
			GENERIC_READ | GENERIC_WRITE,
			&Oa, &Io, 0,
			FILE_ATTRIBUTE_DIRECTORY,
			FILE_SHARE_VALID_FLAGS,
			FILE_OPEN_IF,
			FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			0, 0);
		if (!NT_SUCCESS(Status))
		{
			DbgPrintEx(0, 0, "[LyPdb][%s] create dir C:\\LyPdb\\ error:%x \n", __FUNCTION__, Status);
			return Status;
		}

		ZwClose(h_File);
		return Status;
	}

	kstd::wstring PdbViewer::downLoadPdb(pdbInfo_t& info)
	{
		NTSTATUS status = 0;
		HANDLE hFile = 0;
		wchar_t wPdbName[MAX_PATH] = { 0 };
		wchar_t wPdbGuid[MAX_PATH] = { 0 };
		wchar_t* wRetPdbName = ansiToUni(wPdbName, MAX_PATH, info.second.c_str());
		wchar_t* wRetPdbGuid = ansiToUni(wPdbGuid, MAX_PATH, info.first.c_str());
		kstd::wstring sw_LyPdb_Path = kstd::wstring(L"\\??\\C:\\LyPdb\\");
		UNICODE_STRING u_LyPdb_Path = { 0 };
		kstd::wstring sw_LyPdb_Pdb_Path = sw_LyPdb_Path + wPdbName + kstd::wstring(L"\\");
		UNICODE_STRING u_LyPdb_Pdb_Path = { 0 };
		kstd::wstring sw_LyPdb_Pdb_Guid_Path = sw_LyPdb_Pdb_Path + wRetPdbGuid + kstd::wstring(L"\\");
		UNICODE_STRING u_LyPdb_Pdb_Guid_Path = { 0 };
		kstd::wstring sw_pdbFilePath = sw_LyPdb_Pdb_Guid_Path + wRetPdbName;
		kstd::string sa_pdbFilePath = kstd::string("\\??\\C:\\LyPdb\\") + info.second + kstd::string("\\") + info.first + kstd::string("\\") + info.second;

		// 1. 创建 C:\\LyPdb\\ 目录

		RtlInitUnicodeString(&u_LyPdb_Path, sw_LyPdb_Path.c_str());
		if (!NT_SUCCESS(CreateDirectory(u_LyPdb_Path)))
		{
			DbgPrintEx(0, 0, "[LyPdb][%s] create dir C:\\LyPdb\\ error:%x \n", __FUNCTION__, status);
			return kstd::wstring(L"");
		}

		// 2. 创建 C:\\LyPdb\\*.pdb\\ 目录

		RtlInitUnicodeString(&u_LyPdb_Pdb_Path, sw_LyPdb_Pdb_Path.c_str());
		if (!NT_SUCCESS(CreateDirectory(u_LyPdb_Pdb_Path)))
		{
			DbgPrintEx(0, 0, "[LyPdb][%s] create dir C:\\LyPdb\\Pdb\\ error:%x \n", __FUNCTION__, status);
			return kstd::wstring(L"");
		}

		// 3. 创建 C:\\LyPdb\\pdb\\guid\\ 目录

		RtlInitUnicodeString(&u_LyPdb_Pdb_Guid_Path, sw_LyPdb_Pdb_Guid_Path.c_str());
		if (!NT_SUCCESS(CreateDirectory(u_LyPdb_Pdb_Guid_Path)))
		{
			DbgPrintEx(0, 0, "[LyPdb][%s] create dir C:\\LyPdb\\Pdb\\Guid\\ error:%x \n", __FUNCTION__, status);
			return kstd::wstring(L"");
		}

		// 4. 已存在则不下载

		auto fileExits = isFileExits(sa_pdbFilePath);
		if (fileExits) 
		{
			return sw_pdbFilePath;
		}

		// 5. 获取 pdb url

		kstd::string url = downUrl + info.second + "/" + info.first + "/" + info.second;
		DbgPrintEx(0, 0, "[LyPdb][%s] download url:%s \n",__FUNCTION__,url.c_str());

		// 6. 发送请求 1

		ksocket::init();
		auto pdbSize=ksocket::getContentLength(url.c_str(), "88");
		if (pdbSize == 0)
		{
			DbgPrintEx(0, 0, "[LyPdb][%s] failed to get content length \n",__FUNCTION__);
			return kstd::wstring(L"");
		}

		// 7. 发送请求 2

		auto pdbBuf = kstd::make_unique<unsigned char[]>(pdbSize + 500);
		auto bSuc=ksocket::getHttpContent(url.c_str(), (char*)pdbBuf.get(), pdbSize + 500, "88");
		if (!bSuc) 
		{
			DbgPrintEx(0, 0, "[LyPdb][%s] failed to get pdb file \n",__FUNCTION__);
			return kstd::wstring(L"");
		}
		ksocket::destory();

		// 8. 写入文件

		if (wRetPdbName != wPdbName) 
		{
			DbgPrintEx(0, 0, "[LyPdb][%s] failed to convert ansi to uni \n",__FUNCTION__);
			return kstd::wstring(L"");
		}
		helper::writeToDisk(sw_LyPdb_Pdb_Guid_Path.c_str(), wRetPdbName, (char*)pdbBuf.get(), pdbSize);
		
		return sw_pdbFilePath;
	}

	bool PdbViewer::isFileExits(kstd::string path)
	{
		ANSI_STRING  asPath{ 0 };
		UNICODE_STRING usPath{ 0 };
		OBJECT_ATTRIBUTES oa{ 0 };
		HANDLE hFile{ 0 };
		IO_STATUS_BLOCK iob{ 0 };
		NTSTATUS status = STATUS_UNSUCCESSFUL;

		RtlInitAnsiString(&asPath, path.c_str());
		if (NT_SUCCESS(status=RtlAnsiStringToUnicodeString(&usPath,&asPath, true)))
		{
			InitializeObjectAttributes(&oa, &usPath,  OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

			status = ZwOpenFile(
				&hFile,
				SYNCHRONIZE, &oa,
				&iob,
				FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);

			RtlFreeUnicodeString(&usPath);
		}

		if (NT_SUCCESS(status)) 
		{
			ZwClose(hFile);
			return true; 
		}
		return false;
	}

	wchar_t* PdbViewer::ansiToUni(wchar_t* dest,ULONG size, const char* src)
	{
		UNICODE_STRING uDest{ 0 };
		ANSI_STRING aSrc{ 0 };

		if (dest == nullptr) 
			return 0;

		uDest.Buffer = dest;
		uDest.Length = 0;
		uDest.MaximumLength =(USHORT)((size * sizeof wchar_t));
		RtlInitAnsiString(&aSrc, src);
		if (NT_SUCCESS(RtlAnsiStringToUnicodeString(&uDest, &aSrc, false)))
			return dest;
		else 
			return 0;
	}

}