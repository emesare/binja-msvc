#include <binaryninjaapi.h>

#include "class_hierarchy_descriptor.h"
#include "utils.h"

using namespace BinaryNinja;

Ref<Type> GetClassHierarchyDescriptorType(BinaryView* view)
{
	Ref<Type> typeCache = view->GetTypeById("msvc_RTTIClassHierarchyDescriptor");

	if (typeCache == nullptr)
	{
		Ref<Type> uintType = Type::IntegerType(4, false);
		Ref<Type> intType = Type::IntegerType(4, true);

		StructureBuilder classHierarchyDescriptorBuilder;
		classHierarchyDescriptorBuilder.AddMember(uintType, "signature");
		classHierarchyDescriptorBuilder.AddMember(uintType, "attributes");
		classHierarchyDescriptorBuilder.AddMember(uintType, "numBaseClasses");
		classHierarchyDescriptorBuilder.AddMember(intType, "pBaseClassArray");

		view->DefineType("msvc_RTTIClassHierarchyDescriptor", QualifiedName("_RTTIClassHierarchyDescriptor"),
			TypeBuilder::StructureType(&classHierarchyDescriptorBuilder).Finalize());

		typeCache = view->GetTypeById("msvc_RTTIClassHierarchyDescriptor");
	}

	return typeCache;
}

ClassHierarchyDescriptor::ClassHierarchyDescriptor(BinaryView* view, uint64_t address)
{
	BinaryReader reader = BinaryReader(view);
	reader.Seek(address);

	m_view = view;
	m_address = address;
	m_signatureValue = reader.Read32();
	m_attributesValue = reader.Read32();
	m_numBaseClassesValue = reader.Read32();
	m_pBaseClassArrayValue = (int32_t)reader.Read32();
}

BaseClassArray ClassHierarchyDescriptor::GetBaseClassArray()
{
	return BaseClassArray(m_view, ResolveRelPointer(m_view, m_pBaseClassArrayValue), m_numBaseClassesValue);
}

Ref<Symbol> ClassHierarchyDescriptor::CreateSymbol()
{
	Ref<Symbol> classDescSym = new Symbol {DataSymbol, GetSymbolName(), m_address};
	m_view->DefineUserSymbol(classDescSym);
	m_view->DefineUserDataVariable(m_address, GetClassHierarchyDescriptorType(m_view));
	return classDescSym;
}

// Example: Animal::`RTTI Class Hierarchy Descriptor'
std::string ClassHierarchyDescriptor::GetSymbolName()
{
	return GetBaseClassArray().GetRootClassDescriptor().GetTypeDescriptor().GetDemangledName()
		+ "::`RTTI Class Hierarchy Descriptor'";
}