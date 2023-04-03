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

	LogDebug("Creating symbols for %s...", demangledName.c_str());

	ClassHeirarchyDescriptor classDesc = objLocator.GetClassHeirarchyDescriptor();
	BaseClassArray baseClassArray = classDesc.GetBaseClassArray();

	objLocator.CreateSymbol(demangledName + "_objLocator");
	typeDesc.CreateSymbol(demangledName + "_typeDesc");
	classDesc.CreateSymbol(demangledName + "_classDesc");
	baseClassArray.CreateSymbol(demangledName + "_classArray");

	for (auto&& baseClassDesc : baseClassArray.GetBaseClassDescriptors())
	{
		std::string demangledBaseClassDescName = baseClassDesc.GetTypeDescriptor().GetDemangledName();
		baseClassDesc.CreateSymbol(demangledBaseClassDescName + "_baseClassDesc");
	}
}

void FindAllCOLocators(BinaryView* view)
{
	uint64_t bvStartAddr = view->GetStart();
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
}

extern "C"
{
	BN_DECLARE_CORE_ABI_VERSION

	BINARYNINJAPLUGIN bool CorePluginInit()
	{
		// Ref<Settings> settings = Settings::Instance();
		// settings->RegisterGroup("msvc", "MSVC");
		// settings->RegisterSetting("msvc.autosearch", R"~({
		//     "title" : "Automatically Search",
		//     "type" : "boolean",
		//     "default" : true,
		//     "description" : "If compatible PE binaries should automatically "
		// })~");

		PluginCommand::Register("Find MSVC RTTI", "Scans for all RTTI in view.", &FindAllCOLocators);

		return true;
	}
}