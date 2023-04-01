#pragma once

#include <binaryninjaapi.h>

using namespace BinaryNinja;

// Must be retrieved every time, due to variable length array.
Ref<Type> DefineRTTITypeDescriptor(BinaryView* view, size_t nameLen);

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
	Ref<Symbol> CreateSymbol(std::string name);
};