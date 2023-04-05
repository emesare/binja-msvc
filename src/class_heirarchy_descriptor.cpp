#include <binaryninjaapi.h>

#include "class_heirarchy_descriptor.h"
#include "utils.h"

using namespace BinaryNinja;

Ref<Type> GetClassHeirarchyDescriptorType(BinaryView* view)
{
	Ref<Type> typeCache = view->GetTypeById("msvc_RTTIClassHeirarchyDescriptor");

	if (typeCache == nullptr)
	{
		Ref<Type> uintType = Type::IntegerType(4, false);
		Ref<Type> intType = Type::IntegerType(4, true);

		StructureBuilder classHeirarchyDescriptorBuilder;
		classHeirarchyDescriptorBuilder.AddMember(uintType, "signature");
		classHeirarchyDescriptorBuilder.AddMember(uintType, "attributes");
		classHeirarchyDescriptorBuilder.AddMember(uintType, "numBaseClasses");
		classHeirarchyDescriptorBuilder.AddMember(intType, "pBaseClassArray");

		view->DefineType("msvc_RTTIClassHeirarchyDescriptor", QualifiedName("_RTTIClassHeirarchyDescriptor"),
			TypeBuilder::StructureType(&classHeirarchyDescriptorBuilder).Finalize());

		typeCache = view->GetTypeById("msvc_RTTIClassHeirarchyDescriptor");
	}

	return typeCache;
}

ClassHeirarchyDescriptor::ClassHeirarchyDescriptor(BinaryView* view, uint64_t address)
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

BaseClassArray ClassHeirarchyDescriptor::GetBaseClassArray()
{
	return BaseClassArray(m_view, m_view->GetStart() + m_pBaseClassArrayValue, m_numBaseClassesValue);
}

Ref<Symbol> ClassHeirarchyDescriptor::CreateSymbol(std::string name, std::string rawName)
{
	Ref<Symbol> classDescSym = new Symbol {DataSymbol, name, name, rawName, m_address};
	m_view->DefineUserSymbol(classDescSym);
	m_view->DefineDataVariable(m_address, GetClassHeirarchyDescriptorType(m_view));
	return classDescSym;
}