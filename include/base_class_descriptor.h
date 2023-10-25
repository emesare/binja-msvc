#pragma once

#include <binaryninjaapi.h>

#include "type_descriptor.h"
using namespace BinaryNinja;

Ref<Type> GetBaseClassDescriptorType(BinaryView* view);

class BaseClassDescriptor
{
private:
	Ref<BinaryView> m_view;

public:
	uint64_t m_address;

	// Structure values.
	int32_t m_pTypeDescriptorValue;
	uint32_t m_numContainedBasesValue;
	int32_t m_where_mdispValue;
	int32_t m_where_pdispValue;
	int32_t m_where_vdispValue;
	uint32_t m_attributesValue;
	int32_t m_pClassHierarchyDescriptorValue;

	BaseClassDescriptor(BinaryView* view, uint64_t address);
	TypeDescriptor GetTypeDescriptor();
	Ref<Symbol> CreateSymbol();
	std::string GetSymbolName();
};