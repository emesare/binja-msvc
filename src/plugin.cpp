#include <binaryninjaapi.h>

#include "object_locator.h"
#include "constructor.h"
#include "utils.h"

using namespace BinaryNinja;

// Used to prevent reading over segment bounds.
const size_t COLocatorSize = 24;

void CreateConstructorsAtFunction(BinaryView* view, Function* func)
{
	Constructor constructor = Constructor(view, func);
	// Skip invalid & already tagged constructors.
	if (!constructor.IsValid() || !func->GetFunctionTagsOfType(GetConstructorTagType(view)).empty())
		return;

	// TODO: Apply this to the return type.
	Ref<Type> objType = constructor.GetRootVirtualFunctionTable()->GetObjectType();

	// TODO: Doing any changes to the func here do not get applied...
	auto newVFuncType = [](BinaryView* bv, Ref<Type> funcType, Ref<Type> thisType) {
		auto newFuncType = TypeBuilder(funcType);
		auto adjustedParams = newFuncType.GetParameters();
		if (adjustedParams.empty())
			adjustedParams.push_back({});
		adjustedParams.at(0) = FunctionParameter("this", Type::PointerType(bv->GetAddressSize(), thisType));
		newFuncType.SetParameters(adjustedParams);
		return newFuncType.Finalize();
	};

	func->SetUserType(newVFuncType(view, func->GetType(), objType));

	// Apply to function name.
	constructor.CreateSymbol();
}

void CreateSymbolsFromCOLocatorAddress(BinaryView* view, uint64_t address)
{
	CompleteObjectLocator coLocator = CompleteObjectLocator(view, address);
	if (!coLocator.IsValid())
	{
		LogError("Invalid Colocator! %x", coLocator.m_address);
		return;
	}

	ClassHierarchyDescriptor classDesc = coLocator.GetClassHierarchyDescriptor();
	TypeDescriptor typeDesc = coLocator.GetTypeDescriptor();
	BaseClassArray baseClassArray = classDesc.GetBaseClassArray();
	std::optional<VirtualFunctionTable> vfTable = coLocator.GetVirtualFunctionTable();
	if (!vfTable.has_value())
	{
		LogError("Invalid virtual function table for CoLocator! %x", coLocator.m_address);
		return;
	}

	// Create the classes type.
	auto classTy = vfTable->GetObjectType();

	if (classTy == nullptr)
	{
		LogError("Invalid class type for CoLocator! %x", coLocator.m_address);
		return;
	}

	for (auto&& baseClassDesc : baseClassArray.GetBaseClassDescriptors())
	{
		baseClassDesc.CreateSymbol();
	}

	coLocator.CreateSymbol();
	vfTable->CreateSymbol();
	typeDesc.CreateSymbol();
	classDesc.CreateSymbol();
	baseClassArray.CreateSymbol();

	auto newVFuncType = [](BinaryView* bv, Ref<Type> funcType, Ref<Type> thisType) {
		auto newFuncType = TypeBuilder(funcType);
		auto adjustedParams = newFuncType.GetParameters();
		if (adjustedParams.empty())
			adjustedParams.push_back({});
		adjustedParams.at(0) = FunctionParameter("this", Type::PointerType(bv->GetAddressSize(), thisType));
		newFuncType.SetParameters(adjustedParams);
		return newFuncType.Finalize();
	};

	size_t vFuncIdx = 0;
	auto vftTagType = GetVirtualFunctionTagType(view);
	for (auto&& vFunc : vfTable->GetVirtualFunctions())
	{
		// TODO: Check to see if function already changed by user, if not, don't modify?
		// Must be owned by the class, no inheritence, OR must be unique to the vtable.
		if (vFunc.IsUnique())
		{
			// Remove "Unresolved ownership" tag.
			vFunc.m_func->RemoveUserFunctionTagsOfType(vftTagType);
			vFunc.m_func->CreateUserFunctionTag(vftTagType, "Resolved to " + coLocator.GetClassName(), true);
			vFunc.CreateSymbol(coLocator.GetClassName() + "::vFunc_" + std::to_string(vFuncIdx));
			vFunc.m_func->SetUserType(newVFuncType(view, vFunc.m_func->GetType(), classTy));
		}
		else if (vFunc.m_func->GetUserFunctionTagsOfType(vftTagType).empty())
		{
			vFunc.m_func->CreateUserFunctionTag(vftTagType, "Unresolved ownership", true);
		}
		vFuncIdx++;
	}

	// Add tag to objLocator...
	view->CreateUserDataTag(coLocator.m_address, GetCOLocatorTagType(view), coLocator.GetClassName());
	// Add tag to vfTable...
	view->CreateUserDataTag(vfTable->m_address, GetVirtualFunctionTableTagType(view), vfTable->GetSymbolName());
}

void ScanRTTIView(BinaryView* view)
{
	uint64_t bvStartAddr = view->GetStart();
	std::string undoId = view->BeginUndoActions();
	view->BeginBulkModifySymbols();
	BinaryReader optReader = BinaryReader(view);
	auto addrSize = view->GetAddressSize();

	// Scan data sections for colocators.
	for (Ref<Segment> segment : view->GetSegments())
	{
		if (segment->GetFlags() & (SegmentReadable | SegmentContainsData | SegmentDenyExecute | SegmentDenyWrite))
		{
			LogDebug("Attempting to find CompleteObjectLocators in segment %x", segment->GetStart());
			for (uint64_t currAddr = segment->GetStart(); currAddr < segment->GetEnd() - COLocatorSize;
				 currAddr += addrSize)
			{
				optReader.Seek(currAddr);
				uint32_t sigVal = optReader.Read32();
				if (sigVal == 1 && addrSize == 8)
				{
					optReader.SeekRelative(16);
					if (optReader.Read32() == currAddr - bvStartAddr)
					{
						CreateSymbolsFromCOLocatorAddress(view, currAddr);
					}
				}
				else if (sigVal == 0 && addrSize == 4)
				{
					// Check ?AV
					optReader.SeekRelative(8);
					uint64_t typeDescNameAddr = optReader.Read32() + 8;
					if (typeDescNameAddr > view->GetStart() && typeDescNameAddr < view->GetEnd())
					{
						// Make sure we do not read across segment boundary.
						auto typeDescSegment = view->GetSegmentAt(typeDescNameAddr);
						if (typeDescSegment != nullptr && typeDescSegment->GetEnd() - typeDescNameAddr > 4)
						{
							optReader.Seek(typeDescNameAddr);
							if (optReader.ReadString(4) == ".?AV")
							{
								CreateSymbolsFromCOLocatorAddress(view, currAddr);
							}
						}
					}
				}
			}
		}
	}

	view->EndBulkModifySymbols();
	view->CommitUndoActions(undoId);
	view->Reanalyze();
}

void ScanConstructorView(BinaryView* view)
{
	std::string undoId = view->BeginUndoActions();
	view->BeginBulkModifySymbols();

	std::vector<uint64_t> funcRefs = {};
	for (auto vtableTag : view->GetAllTagReferencesOfType(GetVirtualFunctionTableTagType(view)))
	{
		for (auto codeRef : view->GetCodeReferences(vtableTag.addr))
		{
			uint64_t funcStart = codeRef.func->GetStart();
			if (std::find(funcRefs.begin(), funcRefs.end(), funcStart) != funcRefs.end())
				continue;
			funcRefs.push_back(funcStart);
			CreateConstructorsAtFunction(view, codeRef.func);
		}
	}

	view->EndBulkModifySymbols();
	view->CommitUndoActions(undoId);
	view->Reanalyze();
}

void ScanClassFieldsView(BinaryView* view)
{
	std::string undoId = view->BeginUndoActions();
	view->BeginBulkModifySymbols();

	auto applyFieldAccessesToNamedType = [](BinaryView* bv, Ref<Type> targetType) {
		auto typeName = targetType->GetStructureName();
		bool newMemberAdded = false;
		auto appliedStruct = bv->CreateStructureFromOffsetAccess(typeName, &newMemberAdded);
		if (newMemberAdded)
		{
			bv->DefineUserType(typeName, targetType->WithReplacedStructure(targetType->GetStructure(), appliedStruct));
		}
	};

	for (auto coLocatorTag : view->GetAllTagReferencesOfType(GetCOLocatorTagType(view)))
	{
		auto coLocator = CompleteObjectLocator(view, coLocatorTag.addr);

		for (auto baseClassDesc : coLocator.GetClassHierarchyDescriptor().GetBaseClassArray().GetBaseClassDescriptors())
		{
			auto baseClassType =
				view->GetTypeByName(QualifiedName(baseClassDesc.GetTypeDescriptor().GetDemangledName()));
			if (baseClassType != nullptr)
			{
				applyFieldAccessesToNamedType(view, baseClassType);
			}
		}

		auto classType = view->GetTypeByName(QualifiedName(coLocator.GetClassName()));
		if (classType != nullptr)
		{
			applyFieldAccessesToNamedType(view, classType);
		}
	}

	view->EndBulkModifySymbols();
	view->CommitUndoActions(undoId);
	view->Reanalyze();
}

void GenerateConstructorGraphViz(BinaryView* view)
{
	std::stringstream out;
	out << "digraph Constructors {node [shape=record];\n";

	auto tagType = GetConstructorTagType(view);
	for (auto constructorTag : view->GetAllTagReferencesOfType(tagType))
	{
		auto classNamedType = GetPointerTypeChildStructure(
			constructorTag.func->GetVariableType(constructorTag.func->GetParameterVariables()->front()));
		auto classNamedTypeRef = classNamedType->GetNamedTypeReference();
		if (classNamedType == nullptr || classNamedTypeRef == nullptr)
		{
			LogWarn("class with data (%s) has invalid this param", constructorTag.tag->GetData().c_str());
			continue;
		}

		auto className = classNamedType->GetTypeName().GetString();
		out << '"' << className << '"' << " [label=\"{" << className;

		auto classType = view->GetTypeById(classNamedTypeRef->GetTypeId());
		if (!classType->IsStructure())
		{
			LogWarn("class %s has invalid this param, not a structure...", className.c_str());
			continue;
		}

		out << "}\"];\n";

		auto classTypeStruct = classType->GetStructure();
		for (auto baseStruct : classTypeStruct->GetBaseStructures())
		{
			out << '"' << className << "\"->\"" << baseStruct.type->GetName().GetString() << "\";\n";
		}
	}

	out << "}";
	view->ShowPlainTextReport("MSVC Constructor GraphViz DOT", out.str());
}

void MakeComponents(BinaryView* view)
{
	std::string undoId = view->BeginUndoActions();
	auto classesComp = view->CreateComponentWithName("MSVC Classes");
	for (auto coLocatorTag : view->GetAllTagReferencesOfType(GetCOLocatorTagType(view)))
	{
		auto coLocator = CompleteObjectLocator(view, coLocatorTag.addr);

		auto className = coLocator.GetClassName();
		if (coLocator.IsSubObject())
		{
			className = className + " (" + coLocator.GetAssociatedClassName() + ")";
		}

		auto classComp = view->CreateComponentWithName(className, classesComp->GetGuid());

		DataVariable coLocatorVar = {};
		if (view->GetDataVariableAtAddress(coLocator.m_address, coLocatorVar))
		{
			classComp->AddDataVariable(coLocatorVar);
		}

		if (auto vtable = coLocator.GetVirtualFunctionTable())
		{
			DataVariable vtableVar = {};
			if (view->GetDataVariableAtAddress(coLocator.GetVirtualFunctionTable()->m_address, vtableVar))
			{
				classComp->AddDataVariable(vtableVar);
			}

			for (auto vFunc : vtable->GetVirtualFunctions())
			{
				if (vFunc.IsUnique())
				{
					classComp->AddFunction(vFunc.m_func);
				}
			}
		}
	}
	view->CommitUndoActions(undoId);
}

bool DoesRTTIExist(BinaryView* view)
{
	Ref<TagType> tagType = view->GetTagType("MSVC Complete Object Locator");
	if (tagType != nullptr)
	{
		return true;
	}

	return false;
}

bool CanScanForRTTI(BinaryView* view)
{
	Ref<TagType> tagType = view->GetTagType("MSVC Complete Object Locator");
	if (tagType == nullptr)
	{
		return true;
	}

	return false;
}

extern "C"
{
	BN_DECLARE_CORE_ABI_VERSION

	BINARYNINJAPLUGIN bool CorePluginInit()
	{
		PluginCommand::Register("MSVC\\Find RTTI", "Scans for all RTTI in view.", ScanRTTIView, CanScanForRTTI);
		PluginCommand::Register(
			"MSVC\\Find Constructors", "Scans for all constructors in view.", ScanConstructorView, DoesRTTIExist);
		PluginCommand::Register("MSVC\\Find Class Fields",
			"Scans for all class fields in view. Useful to run after scanning for constructors.", ScanClassFieldsView,
			DoesRTTIExist);
		PluginCommand::Register("MSVC\\Generate Constructors Graphviz",
			"Makes a graph from all the available MSVC constructors.", GenerateConstructorGraphViz, DoesRTTIExist);
		PluginCommand::Register("MSVC\\Make Class Components",
			"Adds relevant data variables and functions to class components.", MakeComponents, DoesRTTIExist);

		return true;
	}
}