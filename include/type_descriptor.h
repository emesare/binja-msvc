#pragma once

#include <binaryninjaapi.h>

using namespace BinaryNinja;

class TypeDescriptor
{
private:
	Ref<BinaryView> m_view;

public:
	uint64_t m_address;

	// Structure values.
	uint64_t m_pVFTableValue;
	uint64_t m_spareValue;
	std::string m_nameValue;

	TypeDescriptor(BinaryView* view, uint64_t address);
	std::string GetDemangledName();
	Ref<Type> GetType();
	Ref<Symbol> CreateSymbol(std::string name, std::string rawName);
};