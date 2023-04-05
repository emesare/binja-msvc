#include <binaryninjaapi.h>
#include <Demangle.h>

#include "object_locator.h"

using namespace BinaryNinja;

// Used to prevent reading over segment bounds.
const size_t COLocatorSize = 24;

void CreateSymbolsFromCOLocatorAddress(BinaryView* view, uint64_t address)
{
	CompleteObjectLocator objLocator = CompleteObjectLocator(view, address);
	if (!objLocator.IsValid())
		return;

	TypeDescriptor typeDesc = objLocator.GetTypeDescriptor();
	std::string demangledName = typeDesc.GetDemangledName();

	ClassHeirarchyDescriptor classDesc = objLocator.GetClassHeirarchyDescriptor();
	VirtualFunctionTable vfTable = objLocator.GetVirtualFunctionTable();
	BaseClassArray baseClassArray = classDesc.GetBaseClassArray();
	std::vector<BaseClassDescriptor> baseClassDescriptors = baseClassArray.GetBaseClassDescriptors();

	// TODO: Cleanup this!
	if (!baseClassDescriptors.empty())
	{
		std::string inheritenceName = " : ";
		bool first = true;
		for (auto&& baseClassDesc : baseClassDescriptors)
		{
			std::string demangledBaseClassDescName = baseClassDesc.GetTypeDescriptor().GetDemangledName();
			baseClassDesc.CreateSymbol(demangledBaseClassDescName + "_baseClassDesc");
			if (demangledBaseClassDescName != demangledName)
			{
				if (first)
				{
					inheritenceName.append(demangledBaseClassDescName);
					first = false;
				}
				else
				{
					inheritenceName.append(", " + demangledBaseClassDescName);
				}
			}
		}

		if (first == false)
		{
			demangledName.append(inheritenceName);
		}
	}

	LogDebug("Creating symbols for %s...", demangledName.c_str());


	// TODO: If option is enabled, create a new structure for this class and define the vtable structures and
	// everything. (so vfuncs are resolved...)

	size_t vFuncIdx = 0;
	for (auto&& vFunc : vfTable.GetVirtualFunctions())
	{
		// rename them, demangledName::funcName
		if (vFunc.IsUnique())
		{
			vFunc.m_func->SetComment("Unique to " + demangledName);
			vFunc.CreateSymbol(demangledName + "::vFunc_" + std::to_string(vFuncIdx));
		}
		else
		{
			// Function used by a lot of vtables, meaning its inheretited
			// TODO: Find the root class for this function and name it for that.
			// TODO: Search baseclassheirarchy?

			// Must be owned by the class, no inheritence.
			if (classDesc.m_numBaseClassesValue <= 1)
			{
				vFunc.CreateSymbol(demangledName + "::vFunc_" + std::to_string(vFuncIdx));
			}
		}
		vFuncIdx++;
	}

	vfTable.CreateSymbol(shortName + "_vfTable", fullName + "_vfTable");
}

void FindAllCOLocators(BinaryView* view)
{
	uint64_t bvStartAddr = view->GetStart();

	view->BeginBulkModifySymbols();

	for (Ref<Segment> segment : view->GetSegments())
	{
		if (segment->GetFlags() & (SegmentReadable | SegmentContainsData | SegmentDenyExecute | SegmentDenyWrite))
		{
			BinaryReader optReader = BinaryReader(view);
			LogDebug("Attempting to find CompleteObjectLocators in segment %x", segment->GetStart());
			for (uint64_t currAddr = segment->GetStart(); currAddr < segment->GetEnd() - COLocatorSize; currAddr++)
			{
				optReader.Seek(currAddr);
				if (optReader.Read32() == 1)
				{
					optReader.SeekRelative(16);
					if (optReader.Read32() == currAddr - bvStartAddr)
					{
						CreateSymbolsFromCOLocatorAddress(view, currAddr);
					}
				}
			}
		}
	}

	view->EndBulkModifySymbols();
}

extern "C"
{
	BN_DECLARE_CORE_ABI_VERSION

	BINARYNINJAPLUGIN bool CorePluginInit()
	{
		// Ref<Settings> settings = Settings::Instance();
		// settings->RegisterGroup("msvc", "MSVC");
		// settings->RegisterSetting("msvc.autosearch", R"~({
		//     "title" : "Automatically Scan RTTI",
		//     "type" : "boolean",
		//     "default" : true,
		//     "description" : "Automatically search and symbolize RTTI information"
		// })~");

		PluginCommand::Register("Find MSVC RTTI", "Scans for all RTTI in view.", &FindAllCOLocators);


		return true;
	}
}