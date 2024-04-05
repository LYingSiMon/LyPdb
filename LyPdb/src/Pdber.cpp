#include "Pdber.hpp"

namespace oxygenPdb 
{
	Pdber::Pdber(const wchar_t* moduleName) :_moduler(moduleName), _initfailed(false), _pdbGuid{ 0 }, _pdbPath{ 0 } {}

	bool Pdber::init()
	{
		const auto [base, size] = _moduler.getModuleInfo();

		do 
		{
			if (base == 0 || size == 0) 
			{
				break;
			}

			// 获取 pdb guid

			auto info = _pdbViewer.getPdbInfo(base);

			// 下载 pdb，或打开现有文件

			const auto& cpath = _pdbViewer.downLoadPdb(info).data();
			if (cpath == 0) 
				break;
			wcscpy_s(_pdbPath, cpath);
			strcpy_s(_pdbGuid, info.first.data());

			// 初始化文件流

			wchar_t wPdbFilePath[MAX_PATH]{ 0 };
			wcscat_s(wPdbFilePath, _pdbPath);
			symbolic_access::FileStream pdbFileStream(wPdbFilePath);
			symbolic_access::MsfReader msfReader(std::move(pdbFileStream));
			if (!msfReader.Initialize())
			{
				DbgPrintEx(0, 0, "[LyPdb][%s] failed to initialize msf reader \n",__FUNCTION__);
				break;
			}

			// 文件流 -> 符号映射

			symbolic_access::SymbolsExtractor symbolsExtractor(msfReader);
			_symbols = symbolsExtractor.Extract();
			if (_symbols.empty())
			{
				DbgPrintEx(0, 0, "[LyPdb][%s] failed to extract symbols for \n",__FUNCTION__);
				break;
			}

			_structs = symbolic_access::StructExtractor(msfReader).Extract();
			return true;

		} while (0);

		DbgPrintEx(0, 0, "[LyPdb][%s] failed to open or download pdb file \n",__FUNCTION__);
		_initfailed = true;
		return false;
	}

	Pdber::~Pdber(){}
}