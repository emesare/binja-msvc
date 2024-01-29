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
	auto coLocator = GetRootVirtualFunctionTable()->GetCOLocator();
	if (!coLocator.has_value())
		return "Constructor_" + std::to_string(GetRootVirtualFunctionTable()->m_address);
	return coLocator->GetTypeDescriptor().GetDemangledName();
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