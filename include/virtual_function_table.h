#pragma once

#include <binaryninjaapi.h>

#include "virtual_function.h"

using namespace BinaryNinja;

class CompleteObjectLocator;

class VirtualFunctionTable
{
private:
	Ref<BinaryView> m_view;

public:
	uint64_t m_address;

	VirtualFunctionTable(BinaryView* view, uint64_t address);
	std::vector<VirtualFunction> GetVirtualFunctions();
	CompleteObjectLocator GetCOLocator();
	Ref<Type> GetType(std::string name, std::string idName);
	Ref<Symbol> CreateSymbol(std::string name, std::string rawName);
};