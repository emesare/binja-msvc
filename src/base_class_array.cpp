#include <binaryninjaapi.h>

#include "class_hierarchy_descriptor.h"
#include "utils.h"

using namespace BinaryNinja;

BaseClassArray::BaseClassArray(BinaryView* view, uint64_t address, int32_t length)
{
	m_view = view;
	m_address = address;
	m_length = length;
}

std::vector<BaseClassDescriptor> BaseClassArray::GetBaseClassDescriptors()
{
	std::vector<BaseClassDescriptor> baseClassDescriptors = {};
	BinaryReader reader = BinaryReader(m_view);
	reader.Seek(m_address);

	for (size_t i = 0; i < m_length; i++)
	{
		baseClassDescriptors.emplace_back(
			BaseClassDescriptor(m_view, ResolveRelPointer(m_view, (uint64_t)reader.Read32())));
	}

	return baseClassDescriptors;
}

BaseClassDescriptor BaseClassArray::GetRootClassDescriptor()
{
	return GetBaseClassDescriptors().front();
}

Ref<Type> BaseClassArray::GetType()
{
	StructureBuilder baseClassArrayBuilder;
	baseClassArrayBuilder.AddMember(
		Type::ArrayType(Type::IntegerType(4, true), m_length), "arrayOfBaseClassDescriptors");

	return TypeBuilder::StructureType(&baseClassArrayBuilder).Finalize();
}

Ref<Symbol> BaseClassArray::CreateSymbol()
{
	Ref<Symbol> baseClassArraySym = new Symbol {DataSymbol, GetSymbolName(), m_address};
	m_view->DefineUserSymbol(baseClassArraySym);
	m_view->DefineUserDataVariable(m_address, GetType());
	return baseClassArraySym;
}

// Example: Animal::`RTTI Base Class Array'
std::string BaseClassArray::GetSymbolName()
{
	return GetRootClassDescriptor().GetTypeDescriptor().GetDemangledName() + "::`RTTI Base Class Array'";
}