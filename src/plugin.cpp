#include <binaryninjaapi.h>
#include <Demangle.h>

#include "object_locator.h"
#include "constructor.h"
#include "utils.h"

using namespace BinaryNinja;

// Used to prevent reading over segment bounds.
const size_t COLocatorSize = 24;

void CreateConstructorsAtFunction(BinaryView* view, Function* func)
{
	Constructor constructor = Constructor(view, func);
	if (!constructor.IsValid())
		return;

	LogDebug("Attempting to create constructor -> %s", constructor.GetName().c_str());

	// Apply this to the return type.
	Ref<Type> objType = constructor.CreateObjectType();
	// TODO: This will immediately go and unmerge in the function body sometimes!
	auto paramVars = func->GetParameterVariables();
	func->CreateUserVariable(paramVars->front(), Type::PointerType(view->GetAddressSize(), objType), "this");

	// TODO: Update all root vfuncs.
	for (auto vFuncs : constructor.GetRootVirtualFunctionTable().value().GetVirtualFunctions())
	{
		auto paramVars = vFuncs.m_func->GetParameterVariables();
		// TODO: This does not add a param like it should...
		if (paramVars->empty())
			paramVars->push_back({});
		vFuncs.m_func->CreateUserVariable(
			paramVars->front(), Type::PointerType(view->GetAddressSize(), objType), "this");
	}

	// Apply to function name.
	constructor.CreateSymbol();

	// Tag function as a constructor.
	func->CreateUserFunctionTag(GetConstructorTagType(view), constructor.GetName());
}

void CreateSymbolsFromCOLocatorAddress(BinaryView* view, uint64_t address)
{
	CompleteObjectLocator objLocator = CompleteObjectLocator(view, address);
	if (!objLocator.IsValid())
		return;

	std::string shortName = objLocator.GetUniqueName();
	std::string rawName = shortName;

	ClassHeirarchyDescriptor classDesc = objLocator.GetClassHeirarchyDescriptor();
	TypeDescriptor typeDesc = objLocator.GetTypeDescriptor();

	std::optional<VirtualFunctionTable> vfTableOpt = objLocator.GetVirtualFunctionTable();
	if (!vfTableOpt.has_value())
	{
		LogWarn("Failed to get VFT for %s", shortName.c_str());
		return;
	}
	VirtualFunctionTable vfTable = vfTableOpt.value();

	BaseClassArray baseClassArray = classDesc.GetBaseClassArray();
	std::vector<BaseClassDescriptor> baseClassDescriptors = baseClassArray.GetBaseClassDescriptors();

	if (objLocator.m_offsetValue != 0)
	{
		rawName = "__offset(" + std::to_string(objLocator.m_offsetValue) + ") " + rawName;
	}

	// TODO: Cleanup this!
	if (!baseClassDescriptors.empty())
	{
		std::string inheritenceName = " : ";
		bool first = true;
		for (auto&& baseClassDesc : baseClassDescriptors)
		{
			std::string demangledBaseClassDescName = baseClassDesc.GetTypeDescriptor().GetDemangledName();
			baseClassDesc.CreateSymbol(demangledBaseClassDescName + "_baseClassDesc");
			if (demangledBaseClassDescName != shortName)
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
			rawName.append(inheritenceName);
		}
	}

	LogDebug("Creating symbols for %s...", shortName.c_str());


	// TODO: If option is enabled, create a new structure for this class and define the vtable structures and
	// everything. (so vfuncs are resolved...)

	size_t vFuncIdx = 0;
	for (auto&& vFunc : vfTable.GetVirtualFunctions())
	{
		// TODO: Check to see if function already changed by user, if not, don't modify?
		// rename them, demangledName::funcName
		if (vFunc.IsUnique())
		{
			vFunc.m_func->SetComment("Unique to " + shortName);
			vFunc.CreateSymbol(
				shortName + "::vFunc_" + std::to_string(vFuncIdx), rawName + "::vFunc_" + std::to_string(vFuncIdx));
		}
		else
		{
			// Must be owned by the class, no inheritence.
			if (classDesc.m_numBaseClassesValue <= 1)
			{
				vFunc.CreateSymbol(
					shortName + "::vFunc_" + std::to_string(vFuncIdx), rawName + "::vFunc_" + std::to_string(vFuncIdx));
			}
		}
		vFuncIdx++;
	}

	// Set comment showing raw name.
	size_t addrSize = view->GetAddressSize();
	std::vector<uint64_t> objLocatorRefs = view->GetDataReferences(objLocator.m_address);
	if (!objLocatorRefs.empty())
		view->SetCommentForAddress(objLocatorRefs.front(), rawName);

	objLocator.CreateSymbol(shortName + "_objLocator", rawName + "_objLocator");
	vfTable.CreateSymbol(shortName + "_vfTable", rawName + "_vfTable");
	typeDesc.CreateSymbol(shortName + "_typeDesc", rawName + "_typeDesc");
	classDesc.CreateSymbol(shortName + "_classDesc", rawName + "_classDesc");
	baseClassArray.CreateSymbol(shortName + "_classArray", rawName + "_classArray");

	// Add tag to vfTable...
	view->CreateUserDataTag(vfTable.m_address, GetVirtualFunctionTableTagType(view), shortName);
}

void ScanRTTIView(BinaryView* view)
{
	uint64_t bvStartAddr = view->GetStart();

	view->BeginUndoActions();
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
	view->CommitUndoActions();
}

void ScanConstructorView(BinaryView* view)
{
	view->BeginUndoActions();
	view->BeginBulkModifySymbols();

	std::vector<uint64_t> doneFuncs = {};
	for (auto vtableTag : view->GetAllTagReferencesOfType(GetVirtualFunctionTableTagType(view)))
	{
		for (auto codeRef : view->GetCodeReferences(vtableTag.addr))
		{
			uint64_t funcStart = codeRef.func->GetStart();
			if (std::find(doneFuncs.begin(), doneFuncs.end(), funcStart) != doneFuncs.end())
				continue;
			doneFuncs.push_back(funcStart);
			CreateConstructorsAtFunction(view, codeRef.func);
		}
	}

	view->EndBulkModifySymbols();
	view->CommitUndoActions();
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

		PluginCommand::Register("Find MSVC RTTI", "Scans for all RTTI in view.", &ScanRTTIView);
		PluginCommand::Register("Find MSVC Constructors", "Scans for all constructors in view.", &ScanConstructorView);

		return true;
	}
}