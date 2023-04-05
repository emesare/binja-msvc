#include <binaryninjaapi.h>

#include "virtual_function_table.h"
#include "utils.h"

using namespace BinaryNinja;

VirtualFunctionTable::VirtualFunctionTable(BinaryView* view, uint64_t vftAddr)
{
	m_view = view;
	m_address = vftAddr;
}

std::vector<VirtualFunction> VirtualFunctionTable::GetVirtualFunctions()
{
	size_t addrSize = m_view->GetAddressSize();
	std::vector<VirtualFunction> vFuncs = {};
	BinaryReader reader = BinaryReader(m_view);
	reader.Seek(m_address);

	while (true)
	{
		uint64_t vFuncAddr = ReadIntWithSize(&reader, addrSize);
		auto funcs = m_view->GetAnalysisFunctionsForAddress(vFuncAddr);
		if (funcs.empty())
		{
			Ref<Segment> segment = m_view->GetSegmentAt(vFuncAddr);
			if (segment == nullptr)
			{
				// Last CompleteObjectLocator?
				break;
			}

			if (segment->GetFlags() & (SegmentExecutable | SegmentDenyWrite))
			{
				LogInfo("Discovered function from vtable reference -> %x", vFuncAddr);
				m_view->CreateUserFunction(m_view->GetDefaultPlatform(), vFuncAddr);
				funcs = m_view->GetAnalysisFunctionsForAddress(vFuncAddr);
			}
			else
			{
				// Hit the next CompleteObjectLocator
				break;
			}
		}

		vFuncs.emplace_back(VirtualFunction(m_view, vFuncAddr, funcs.front()));
	}


	return vFuncs;
}

// TODO: This type is not unique, I think we should make rawname include a __offset(0x8) thing before it to denote the
// objectLocators offset.
Ref<Type> VirtualFunctionTable::GetType(std::string idName)
{
	Ref<Type> typeCache = m_view->GetTypeById("msvc_" + idName);

	if (typeCache == nullptr)
	{
		size_t addrSize = m_view->GetAddressSize();
		StructureBuilder vftBuilder;
		size_t vFuncIdx = 0;
		for (auto&& vFunc : GetVirtualFunctions())
		{
			vftBuilder.AddMember(
				Type::PointerType(addrSize, vFunc.m_func->GetType(), true), "vFunc_" + std::to_string(vFuncIdx));
			vFuncIdx++;
		}

		m_view->DefineType("msvc_" + idName, QualifiedName(idName), TypeBuilder::StructureType(&vftBuilder).Finalize());

		typeCache = TypeBuilder::StructureType(&vftBuilder).Finalize();
	}

	return typeCache;
}

Ref<Symbol> VirtualFunctionTable::CreateSymbol(std::string name, std::string rawName)
{
	Ref<Symbol> newFuncSym = new Symbol {DataSymbol, name, name, rawName, m_address};
	m_view->DefineUserSymbol(newFuncSym);
	m_view->DefineDataVariable(m_address, GetType(rawName));
	return newFuncSym;
}