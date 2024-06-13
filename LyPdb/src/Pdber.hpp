#pragma once
/*
Moduler is a product class in the Factory Pattern, 
and in combination with the PDB Viewer class, 
it can parse the symbols within the module.
*/
#include <fltKernel.h>
#include <kstl/memory.h>
#include "viewer.hpp"
#include <kstl/string.h>
#include "moduler.hpp"
#include "./air14/FileStream.h"
#include "./air14/MsfReader.h"
#include "./air14/SymbolExtractor.h"
#include "./air14/StructExtractor.h"

namespace oxygenPdb 
{
	class Pdber 
	{
		
	public:
		Pdber(const wchar_t* moduleName, bool isR3, bool IsWow64);
		~Pdber();

		bool init();
		ULONG_PTR GetPointer(const char* name);
		size_t GetOffset(const char* structName, const char* propertyName);

	private:
		using ptr_t = UINT_PTR;
		PdbViewer pdbViewer;
		Moduler moduler;
		char pdbGuid[100];
		wchar_t pdbPath[MAX_PATH];
		symbolic_access::SymbolsMap symbols;
		symbolic_access::StructsMap structs;
		bool _initfailed;
	};

	ULONG_PTR Pdber::GetPointer(const char* name)
	{
		if (_initfailed) return 0;
		std::string_view SymbolName(name);

		const auto& iter = symbols.find(SymbolName);
		if (iter == symbols.end())
			return 0;

		return (ULONG_PTR)(moduler.getModuleBase() + iter->second);
	}

	size_t Pdber::GetOffset(const char* structName, const char* propertyName) {

		const auto& structsIter = this->structs.find(structName);
		if (structsIter == structs.end())
			return 0;

		const auto& membersIter = std::find_if(structsIter->second.begin(), structsIter->second.end(),
			[&](const auto& MemberNameAndOffset) { return MemberNameAndOffset.Name == propertyName; });

		if (membersIter == structsIter->second.end())
			return 0;

		return membersIter->Offset;
	}

}