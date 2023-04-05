#include <binaryninjaapi.h>

#include "object_locator.h"
#include "utils.h"

using namespace BinaryNinja;

Ref<Type> GetCompleteObjectLocatorType(BinaryView* view)
{
	Ref<Type> typeCache = view->GetTypeById("msvc_RTTICompleteObjectLocator");

	if (typeCache == nullptr)
	{
		Ref<Type> uintType = Type::IntegerType(4, false);
		Ref<Type> intType = Type::IntegerType(4, true);

		StructureBuilder completeObjectLocatorBuilder;
		// TODO: make signature an enum with COL_SIG_REV0 & COL_SIG_REV1?
		completeObjectLocatorBuilder.AddMember(uintType, "signature");
		completeObjectLocatorBuilder.AddMember(uintType, "offset");
		completeObjectLocatorBuilder.AddMember(uintType, "cdOffset");
		completeObjectLocatorBuilder.AddMember(intType, "pTypeDescriptor");
		completeObjectLocatorBuilder.AddMember(intType, "pClassHeirarchyDescriptor");
		completeObjectLocatorBuilder.AddMember(intType, "pSelf");

		view->DefineType("msvc_RTTICompleteObjectLocator", QualifiedName("_RTTICompleteObjectLocator"),
			TypeBuilder::StructureType(&completeObjectLocatorBuilder).Finalize());

		typeCache = view->GetTypeById("msvc_RTTICompleteObjectLocator");
	}

	return typeCache;
}

CompleteObjectLocator::CompleteObjectLocator(BinaryView* view, uint64_t address)
{
	BinaryReader reader = BinaryReader(view);
	reader.Seek(address);

	m_view = view;
	m_address = address;
	m_signatureValue = reader.Read32();
	m_offsetValue = reader.Read32();
	m_cdOffsetValue = reader.Read32();
	m_pTypeDescriptorValue = (int32_t)reader.Read32();
	m_pClassHeirarchyDescriptorValue = (int32_t)reader.Read32();
	m_pSelfValue = (int32_t)reader.Read32();
}

TypeDescriptor CompleteObjectLocator::GetTypeDescriptor()
{
	if (m_signatureValue == COL_SIG_REV1)
		return TypeDescriptor(m_view, m_view->GetStart() + m_pTypeDescriptorValue);
	return TypeDescriptor(m_view, m_pTypeDescriptorValue);
}

ClassHeirarchyDescriptor CompleteObjectLocator::GetClassHeirarchyDescriptor()
{
	if (m_signatureValue == COL_SIG_REV1)
		return ClassHeirarchyDescriptor(m_view, m_view->GetStart() + m_pClassHeirarchyDescriptorValue);
	return ClassHeirarchyDescriptor(m_view, m_pClassHeirarchyDescriptorValue);
}

VirtualFunctionTable CompleteObjectLocator::GetVirtualFunctionTable()
{
	size_t addrSize = m_view->GetAddressSize();
	std::vector<uint64_t> objLocatorRefs = m_view->GetDataReferences(m_address);
	return VirtualFunctionTable(m_view, objLocatorRefs.front() + addrSize);
}

bool CompleteObjectLocator::IsValid()
{
	uint64_t startAddr = m_view->GetStart();
	uint64_t endAddr = m_view->GetEnd();

	if (m_signatureValue > 1)
		return false;

	if (m_signatureValue == COL_SIG_REV1)
	{
		// Relative addrs
		if (m_pTypeDescriptorValue + startAddr > endAddr)
			return false;

		if (m_pClassHeirarchyDescriptorValue + startAddr > endAddr)
			return false;
	}
	else
	{
		// Absolute addrs
		if (m_pTypeDescriptorValue < startAddr || m_pTypeDescriptorValue > endAddr)
			return false;

		if (m_pClassHeirarchyDescriptorValue < startAddr || m_pClassHeirarchyDescriptorValue > endAddr)
			return false;
	}

	if (m_pSelfValue != m_address - startAddr)
		return false;

	return true;
}

Ref<Symbol> CompleteObjectLocator::CreateSymbol(std::string name)
{

Ref<Symbol> CompleteObjectLocator::CreateSymbol(std::string name, std::string rawName)
{
	Ref<Symbol> COLocSym = new Symbol {DataSymbol, name, name, rawName, m_address};
	m_view->DefineUserSymbol(COLocSym);
	m_view->DefineDataVariable(m_address, GetCompleteObjectLocatorType(m_view));
	return COLocSym;
}