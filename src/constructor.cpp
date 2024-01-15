#include <binaryninjaapi.h>
#include <mediumlevelilinstruction.h>

#include "constructor.h"
#include "utils.h"

using namespace BinaryNinja;

Constructor::Constructor(BinaryView* view, Function* func)
{
	m_view = view;
	m_func = func;
}

bool Constructor::IsValid()
{
	// TODO: Check to make sure we have the first param as a _vfTable** type...
	// TODO: x86 getting constant pointer recognition failures, see: binaryninja-api issue 4399.
	return GetRootVirtualFunctionTable().has_value();
}

std::string Constructor::GetName()
{
	return GetRootVirtualFunctionTable()->GetCOLocator()->GetTypeDescriptor().GetDemangledName();
}

std::optional<VirtualFunctionTable> Constructor::GetRootVirtualFunctionTable()
{
	Ref<TagType> vftTagType = GetVirtualFunctionTableTagType(m_view);
	Ref<MediumLevelILFunction> mlil = m_func->GetMediumLevelIL()->GetSSAForm();

	for (auto& block : mlil->GetBasicBlocks())
	{
		for (size_t instIdx = block->GetStart(); instIdx < block->GetEnd(); instIdx++)
		{
			MediumLevelILInstruction inst = (*mlil)[instIdx];

			if (inst.operation == MLIL_STORE_SSA)
			{
				auto destExpr = inst.GetDestExpr<MLIL_STORE_SSA>();
				auto sourceExpr = inst.GetSourceExpr<MLIL_STORE_SSA>();
				DataVariable sourceDataVar = {};

				if (m_view->GetDataVariableAtAddress(sourceExpr.GetValue().value, sourceDataVar))
				{
					for (auto& tag : m_view->GetDataTags(sourceDataVar.address))
					{
						if (tag->GetType() != vftTagType)
							continue;

						switch (destExpr.operation)
						{
						case MLIL_VAR_SSA:
							return VirtualFunctionTable(m_view, sourceDataVar.address);
						}
					}
				}
			}
		}
	}

	return std::nullopt;
}

size_t Constructor::AddTag()
{
	auto tagType = GetConstructorTagType(m_view);
	auto constructorName = GetName();
	m_func->CreateUserFunctionTag(tagType, constructorName, true);

	size_t tagCount = 0;
	for (auto funcTags : m_view->GetAllTagReferencesOfType(tagType))
	{
		if (funcTags.tag->GetData() == constructorName)
		{
			tagCount++;
		}
	}
	return tagCount;
}

Ref<Type> Constructor::CreateObjectType()
{
	Ref<TagType> vftTagType = GetVirtualFunctionTableTagType(m_view);
	QualifiedName typeName = QualifiedName(GetName());
	Ref<Type> typeCache = Type::NamedType(m_view, typeName);

	// TODO: What if one constructor has fields defined that another one doesnt, does that even happen? TLDR: We need a
	// way to add fields to already existing constructor.

	// TODO: We need to get fields, we should assume the ownership of fields by using the bases offset to put all fields
	// that are below the next vtable to the last assigned vtable

	if (m_view->GetTypeByName(typeName) == nullptr)
	{
		StructureBuilder objBuilder = {};
		objBuilder.SetStructureType(ClassStructureType);
		Ref<MediumLevelILFunction> mlil = m_func->GetMediumLevelIL()->GetSSAForm();

		// Collect all object vtables and set their appropriate types.
		for (auto& block : mlil->GetBasicBlocks())
		{
			for (size_t instIdx = block->GetStart(); instIdx < block->GetEnd(); instIdx++)
			{
				MediumLevelILInstruction inst = (*mlil)[instIdx];

				if (inst.operation == MLIL_STORE_SSA)
				{
					auto destExpr = inst.GetDestExpr<MLIL_STORE_SSA>();
					auto sourceExpr = inst.GetSourceExpr<MLIL_STORE_SSA>();
					DataVariable sourceDataVar = {};
					if (m_view->GetDataVariableAtAddress(sourceExpr.GetValue().value, sourceDataVar))
					{
						for (auto& tag : m_view->GetDataTags(sourceDataVar.address))
						{
							if (tag->GetType() != vftTagType)
								continue;

							int64_t offset = 0;
							switch (destExpr.operation)
							{
							case MLIL_ADD:
								auto leftExpr = destExpr.GetLeftExpr<MLIL_ADD>();
								auto rightExpr = destExpr.GetRightExpr<MLIL_ADD>();
								offset = rightExpr.GetValue().value;
								break;
							}

							VirtualFunctionTable currentVft = VirtualFunctionTable(m_view, sourceDataVar.address);
							auto currentVftCOLocator = currentVft.GetCOLocator();

							if (offset != 0 && currentVftCOLocator.has_value())
							{
								objBuilder.AddMemberAtOffset(
									Type::PointerType(m_view->GetAddressSize(), sourceDataVar.type),
									"vtable_" + currentVftCOLocator->GetAssociatedClassName(), offset);
							}
							else
							{
								objBuilder.AddMemberAtOffset(
									Type::PointerType(m_view->GetAddressSize(), sourceDataVar.type), "vtable", offset);
							}
						}
					}
				}
			}
		}

		std::vector<BaseStructure> innerStructures = {};
		for (auto callSite : m_func->GetCallSites())
		{
			for (auto calleeAddr : m_view->GetCallees(callSite))
			{
				auto calleeFuncs = m_view->GetAnalysisFunctionsForAddress(calleeAddr);
				if (calleeFuncs.empty())
					continue;

				auto innerConstructor = Constructor(m_view, calleeFuncs.front());
				if (!innerConstructor.IsValid())
					continue;

				std::vector<VariableReferenceSource> varRefs =
					m_func->GetMediumLevelILVariableReferencesFrom(m_view->GetDefaultArchitecture(), callSite.addr);
				if (varRefs.empty())
					continue;

				// TODO: What happens if a field exists in an inherited class and the root class? (ANSWER: It is from
				// the inherited class!)
				auto innerTy = innerConstructor.CreateObjectType();

				innerStructures.emplace_back(BaseStructure(innerTy->GetNamedTypeReference(),
					innerConstructor.GetRootVirtualFunctionTable()->GetCOLocator()->m_offsetValue,
					innerTy->GetWidth()));
			}
		}
		objBuilder.SetBaseStructures(innerStructures);

		m_view->DefineUserType(typeName, TypeBuilder::StructureType(&objBuilder).Finalize());

		typeCache = Type::NamedType(m_view, typeName);
	}

	return typeCache;
}

Ref<Symbol> Constructor::CreateSymbol()
{
	Ref<Symbol> newFuncSym = new Symbol {FunctionSymbol, GetSymbolName(), m_func->GetStart()};
	m_view->DefineUserSymbol(newFuncSym);
	return newFuncSym;
}

std::string Constructor::GetSymbolName()
{
	std::string symName = GetName();
	// If this constructor is not the first of its class, add a counter to the end of it.
	size_t tagCount = AddTag();
	if (tagCount > 1)
	{
		return symName + "::" + symName + "_" + std::to_string(tagCount);
	}
	return symName + "::" + symName;
}