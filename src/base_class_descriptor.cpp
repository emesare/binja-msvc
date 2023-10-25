#include <binaryninjaapi.h>

#include "base_class_descriptor.h"
#include "utils.h"

using namespace BinaryNinja;

Ref<Type> GetPMDType(BinaryView* view)
{
	Ref<Type> typeCache = view->GetTypeById("msvc_PMD");

	if (typeCache == nullptr)
	{
		Ref<Type> intType = Type::IntegerType(4, true);

		StructureBuilder pmdBuilder;
		pmdBuilder.AddMember(intType, "mdisp");
		pmdBuilder.AddMember(intType, "pdisp");
		pmdBuilder.AddMember(intType, "vdisp");

		view->DefineType("msvc_PMD", QualifiedName("_PMD"), TypeBuilder::StructureType(&pmdBuilder).Finalize());
		typeCache = view->GetTypeById("msvc_PMD");
	}

	return typeCache;
}

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
		baseClassDescriptorBuilder.AddMember(GetPMDType(view), "where");
		baseClassDescriptorBuilder.AddMember(uintType, "attributes");
		baseClassDescriptorBuilder.AddMember(intType, "pClassDescriptor");

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
	m_pClassHierarchyDescriptorValue = (int32_t)reader.Read32();
}

TypeDescriptor BaseClassDescriptor::GetTypeDescriptor()
{
	return TypeDescriptor(m_view, ResolveRelPointer(m_view, m_pTypeDescriptorValue));
}

Ref<Symbol> BaseClassDescriptor::CreateSymbol()
{
	Ref<Symbol> baseClassDescriptorSym = new Symbol {DataSymbol, GetSymbolName(), m_address};
	m_view->DefineUserSymbol(baseClassDescriptorSym);
	m_view->DefineUserDataVariable(m_address, GetBaseClassDescriptorType(m_view));
	return baseClassDescriptorSym;
}

// Example: Animal::`RTTI Base Class Descriptor at (0,-1,0,64)'
std::string BaseClassDescriptor::GetSymbolName()
{
	return GetTypeDescriptor().GetDemangledName() + "::`RTTI Base Class Descriptor at ("
		+ std::to_string(m_where_mdispValue) + "," + std::to_string(m_where_pdispValue) + ","
		+ std::to_string(m_where_vdispValue) + "," + std::to_string(m_attributesValue) + ")'";
}