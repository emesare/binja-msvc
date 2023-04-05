#pragma once

#include <binaryninjaapi.h>

#include "virtual_function.h"

using namespace BinaryNinja;

class VirtualFunctionTable
{
private:
	Ref<BinaryView> m_view;

public:
	uint64_t m_address;

	VirtualFunctionTable(BinaryView* view, uint64_t address);
	std::vector<VirtualFunction> GetVirtualFunctions();
	Ref<Type> GetType();
	Ref<Symbol> CreateSymbol(std::string name, std::string rawName);
};