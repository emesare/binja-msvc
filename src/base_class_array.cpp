#include <binaryninjaapi.h>

#include "class_heirarchy_descriptor.h"

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
		baseClassDescriptors.emplace_back(BaseClassDescriptor(m_view, m_view->GetStart() + reader.Read32()));
	}

	return baseClassDescriptors;
}

Ref<Type> BaseClassArray::GetType()
{
	StructureBuilder baseClassArrayBuilder;
	baseClassArrayBuilder.AddMember(
		Type::ArrayType(Type::IntegerType(4, true), m_length), "arrayOfBaseClassDescriptors");

	return TypeBuilder::StructureType(&baseClassArrayBuilder).Finalize();
}

Ref<Symbol> BaseClassArray::CreateSymbol(std::string name, std::string rawName)
{
	Ref<Symbol> baseClassArraySym = new Symbol {DataSymbol, name, name, rawName, m_address};
	m_view->DefineUserSymbol(baseClassArraySym);
	m_view->DefineDataVariable(m_address, GetType());
	return baseClassArraySym;
}