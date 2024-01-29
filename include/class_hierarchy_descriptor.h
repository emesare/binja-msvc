#pragma once

#include <binaryninjaapi.h>

#include "base_class_array.h"

using namespace BinaryNinja;

Ref<Type> GetClassHierarchyDescriptorType();

class ClassHierarchyDescriptor
{
private:
	Ref<BinaryView> m_view;

public:
	uint64_t m_address;

	// Structure values.
	uint32_t m_signatureValue;
	uint32_t m_attributesValue;
	uint32_t m_numBaseClassesValue;
	int32_t m_pBaseClassArrayValue;

	ClassHierarchyDescriptor(BinaryView* view, uint64_t address);
	BaseClassArray GetBaseClassArray();
	Ref<Symbol> CreateSymbol();
	std::string GetSymbolName();
};