#include <binaryninjaapi.h>

#include "virtual_function.h"

using namespace BinaryNinja;

VirtualFunction::VirtualFunction(BinaryView* view, uint64_t vftAddr, Ref<Function> func)
{
	m_view = view;
	m_vftAddr = vftAddr;
	m_func = func;
}

bool VirtualFunction::IsUnique()
{
	return m_view->GetDataReferences(m_func->GetStart()).size() == 1;
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