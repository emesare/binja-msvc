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
	std::string GetRawName();

public:
	Constructor(BinaryView* view, Function* func);
	bool IsValid();
	std::string GetName();
	std::vector<Constructor> GetInnerConstructors();
	std::optional<VirtualFunctionTable> GetRootVirtualFunctionTable();
	Ref<Type> CreateObjectType();
	Ref<Symbol> CreateSymbol();
};