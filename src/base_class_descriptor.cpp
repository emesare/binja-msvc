#include <binaryninjaapi.h>

#include "base_class_descriptor.h"

using namespace BinaryNinja;

Ref<Type> GetBaseClassDescriptorType(BinaryView* view)
{
	Ref<Type> typeCache = view->GetTypeById("msvc_RTTIBaseClassDescriptor");

	if (typeCache == nullptr)
	{
		Ref<Type> uintType = Type::IntegerType(4, false);
		Ref<Type> intType = Type::IntegerType(4, true);

		StructureBuilder baseClassDescriptorBuilder;
		baseClassDescriptorBuilder.AddMember(intType, "pTypeDescriptor");
		baseClassDescriptorBuilder.AddMember(uintType, "numContainedBases");
		baseClassDescriptorBuilder.AddMember(intType, "where_mdisp");
		baseClassDescriptorBuilder.AddMember(intType, "where_pdisp");
		baseClassDescriptorBuilder.AddMember(intType, "where_vdisp");
		baseClassDescriptorBuilder.AddMember(uintType, "attributes");
		baseClassDescriptorBuilder.AddMember(intType, "pClassHeirarchyDescriptor");

		view->DefineType("msvc_RTTIBaseClassDescriptor", QualifiedName("_RTTIBaseClassDescriptor"),
			TypeBuilder::StructureType(&baseClassDescriptorBuilder).Finalize());

		typeCache = view->GetTypeById("msvc_RTTIBaseClassDescriptor");
	}

	return typeCache;
}

BaseClassDescriptor::BaseClassDescriptor(BinaryView* view, uint64_t address)
{
	BinaryReader reader = BinaryReader(view);
	reader.Seek(address);

	m_view = view;
	m_address = address;
	m_pTypeDescriptorValue = (int32_t)reader.Read32();
	m_numContainedBasesValue = reader.Read32();
	m_where_mdispValue = (int32_t)reader.Read32();
	m_where_pdispValue = (int32_t)reader.Read32();
	m_where_vdispValue = (int32_t)reader.Read32();
	m_attributesValue = reader.Read32();
	m_pClassHeirarchyDescriptorValue = (int32_t)reader.Read32();
}

TypeDescriptor BaseClassDescriptor::GetTypeDescriptor()
{
	// NOTE: No signature value attached to `BaseClassDescriptor` so must be relative?
	return TypeDescriptor(m_view, m_view->GetStart() + m_pTypeDescriptorValue);
}

Ref<Symbol> BaseClassDescriptor::CreateSymbol(std::string name)
{
	Ref<Symbol> baseClassDescriptorSym = new Symbol {DataSymbol, name, m_address};
	return m_view->DefineAutoSymbolAndVariableOrFunction(
		m_view->GetDefaultPlatform(), baseClassDescriptorSym, GetBaseClassDescriptorType(m_view));
}