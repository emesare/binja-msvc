#pragma once

#include <binaryninjaapi.h>

#include "type_descriptor.h"
#include "class_hierarchy_descriptor.h"
#include "virtual_function_table.h"

using namespace BinaryNinja;

constexpr auto COL_SIG_REV1 = 1;

class CompleteObjectLocator
{
private:
	Ref<BinaryView> m_view;

public:
	uint64_t m_address;

	// Structure values.
	uint32_t m_signatureValue;
	uint32_t m_offsetValue;
	uint32_t m_cdOffsetValue;
	int32_t m_pTypeDescriptorValue;
	int32_t m_pClassHierarchyDescriptorValue;
	int32_t m_pSelfValue;

	CompleteObjectLocator(BinaryView* view, uint64_t address);
	TypeDescriptor GetTypeDescriptor();
	ClassHierarchyDescriptor GetClassHierarchyDescriptor();
	std::optional<VirtualFunctionTable> GetVirtualFunctionTable();
	bool IsValid();
	bool IsSubObject();
	std::optional<TypeDescriptor> GetSubObjectTypeDescriptor();
	Ref<Type> GetType();
	std::string GetAssociatedClassName();
	Ref<Symbol> CreateSymbol();
	std::string GetSymbolName();
	std::string GetClassName();
};