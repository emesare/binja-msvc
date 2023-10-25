#pragma once

#include <binaryninjaapi.h>

#include "virtual_function_table.h"
#include "object_locator.h"

using namespace BinaryNinja;

class Constructor
{
private:
	Ref<BinaryView> m_view;
	Ref<Function> m_func;

public:
	Constructor(BinaryView* view, Function* func);
	bool IsValid();
	std::string GetName();
	std::optional<VirtualFunctionTable> GetRootVirtualFunctionTable();
	size_t AddTag();
	Ref<Type> CreateObjectType();
	Ref<Symbol> CreateSymbol();
	std::string GetSymbolName();
};