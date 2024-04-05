#include <ntifs.h>

namespace oxygenPdb
{
	class Pdber
	{
	public:
		Pdber(const wchar_t* moduleName);
		~Pdber();
		bool init();
		ULONG_PTR GetPointer(const char* name);
		size_t GetOffset(const char* structName, const char* propertyName);
	private:
		char padding[1000];
	};
}

EXTERN_C auto DriverEntry(PDRIVER_OBJECT drv, PUNICODE_STRING) 
{
	drv->DriverUnload = [](PDRIVER_OBJECT)->void
	{
		
	};

	oxygenPdb::Pdber ntos(L"ntoskrnl.exe");
	ntos.init();

	auto p = ntos.GetPointer("ZwCreateThread");
	auto o = ntos.GetOffset("_KTHREAD", "PreviousMode");

	DbgPrintEx(0, 0, "[LyPdb][%s] p->%llx , o->%llx \n", __FUNCTION__, p, o);

	return STATUS_SUCCESS;
}