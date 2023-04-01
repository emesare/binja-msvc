#pragma once

#include <binaryninjaapi.h>

#include "base_class_descriptor.h"

using namespace BinaryNinja;

class BaseClassArray
{
private:
	Ref<BinaryView> m_view;

public:
	uint64_t m_address;
	int32_t m_length;

	BaseClassArray(BinaryView* view, uint64_t address, int32_t length);
	std::vector<BaseClassDescriptor> GetBaseClassDescriptors();
	Ref<Type> GetType();
	Ref<Symbol> CreateSymbol(std::string name);
};