#include <binaryninjaapi.h>

#include "virtual_function.h"
#include "virtual_function_table.h"
#include "object_locator.h"

using namespace BinaryNinja;

VirtualFunction::VirtualFunction(BinaryView* view, uint64_t vftAddr, Ref<Function> func)
{
	m_view = view;
	m_vftAddr = vftAddr;
	m_func = func;
}

bool VirtualFunction::IsUnique()
{
	auto dataRefs = m_view->GetDataReferences(m_func->GetStart());
	if (dataRefs.size() == 1)
		return true;
	int classRoots = 0;
	for (auto dataRefAddr : dataRefs)
	{
		if (classRoots > 1)
			break;
		auto vfTable = VirtualFunctionTable(m_view, dataRefAddr);
		if (auto coLocator = vfTable.GetCOLocator())
		{
			if (coLocator->IsValid() && coLocator->GetClassHierarchyDescriptor().m_numBaseClassesValue <= 1)
			{
				classRoots++;
			}
		}
	}
	return classRoots == 1;
}

// TODO: IsThunk
// TODO: IsConstructor?
// TODO: IsDestructor?

Ref<Symbol> VirtualFunction::CreateSymbol(std::string name)
{
	Ref<Symbol> newFuncSym = new Symbol {FunctionSymbol, name, m_func->GetStart()};
	m_view->DefineUserSymbol(newFuncSym);
	return newFuncSym;
}