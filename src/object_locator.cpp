#include <binaryninjaapi.h>

#include "object_locator.h"
#include "utils.h"

using namespace BinaryNinja;

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
	m_pClassHierarchyDescriptorValue = (int32_t)reader.Read32();
	if (m_signatureValue == COL_SIG_REV1)
	{
		m_pSelfValue = (int32_t)reader.Read32();
	}
	else
	{
		m_pSelfValue = 0;
	}
}

TypeDescriptor CompleteObjectLocator::GetTypeDescriptor()
{
	if (m_signatureValue == COL_SIG_REV1)
		return TypeDescriptor(m_view, m_view->GetStart() + m_pTypeDescriptorValue);
	return TypeDescriptor(m_view, m_pTypeDescriptorValue);
}

ClassHierarchyDescriptor CompleteObjectLocator::GetClassHierarchyDescriptor()
{
	if (m_signatureValue == COL_SIG_REV1)
		return ClassHierarchyDescriptor(m_view, m_view->GetStart() + m_pClassHierarchyDescriptorValue);
	return ClassHierarchyDescriptor(m_view, m_pClassHierarchyDescriptorValue);
}

std::optional<VirtualFunctionTable> CompleteObjectLocator::GetVirtualFunctionTable()
{
	size_t addrSize = m_view->GetAddressSize();
	std::vector<uint64_t> objLocatorRefs = m_view->GetDataReferences(m_address);
	if (objLocatorRefs.empty())
		return std::nullopt;
	return std::optional(VirtualFunctionTable(m_view, objLocatorRefs.front() + addrSize));
}

bool CompleteObjectLocator::IsValid()
{
	uint64_t startAddr = m_view->GetStart();
	uint64_t endAddr = m_view->GetEnd();

	if (m_signatureValue > 1)
		return false;

	if (m_signatureValue == COL_SIG_REV1)
	{
		if (m_pSelfValue != m_address - startAddr)
			return false;

		// Relative addrs
		if (m_pTypeDescriptorValue + startAddr > endAddr)
			return false;

		if (m_pClassHierarchyDescriptorValue + startAddr > endAddr)
			return false;
	}
	else
	{
		// Absolute addrs
		if (m_pTypeDescriptorValue < startAddr || m_pTypeDescriptorValue > endAddr)
			return false;

		if (m_pClassHierarchyDescriptorValue < startAddr || m_pClassHierarchyDescriptorValue > endAddr)
			return false;
	}

	return true;
}

bool CompleteObjectLocator::IsSubObject()
{
	return m_offsetValue > 0;
}

// TODO: This fails sometimes, figure out what causes this.
std::optional<TypeDescriptor> CompleteObjectLocator::GetSubObjectTypeDescriptor()
{
	if (!IsSubObject())
		return std::nullopt;

	for (auto baseClassDescs : GetClassHierarchyDescriptor().GetBaseClassArray().GetBaseClassDescriptors())
	{
		if ((int32_t)m_offsetValue == baseClassDescs.m_where_mdispValue)
		{
			return baseClassDescs.GetTypeDescriptor();
		}
	}

	return std::nullopt;
}

Ref<Type> CompleteObjectLocator::GetType()
{
	auto sigValStr = std::to_string(m_signatureValue);
	Ref<Type> typeCache = m_view->GetTypeById("msvc_RTTICompleteObjectLocator" + sigValStr);

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
		completeObjectLocatorBuilder.AddMember(intType, "pClassHierarchyDescriptor");

		if (m_signatureValue == COL_SIG_REV1)
		{
			completeObjectLocatorBuilder.AddMember(intType, "pSelf");
		}

		m_view->DefineType("msvc_RTTICompleteObjectLocator" + sigValStr, QualifiedName("_RTTICompleteObjectLocator"),
			TypeBuilder::StructureType(&completeObjectLocatorBuilder).Finalize());

		typeCache = m_view->GetTypeById("msvc_RTTICompleteObjectLocator" + sigValStr);
	}

	return typeCache;
}

std::string CompleteObjectLocator::GetAssociatedClassName()
{
	if (auto subObjectTypeDesc = GetSubObjectTypeDescriptor())
	{
		return subObjectTypeDesc->GetDemangledName();
	}
	return GetTypeDescriptor().GetDemangledName();
}

Ref<Symbol> CompleteObjectLocator::CreateSymbol()
{
	Ref<Symbol> COLocSym = new Symbol {DataSymbol, GetSymbolName(), m_address};
	m_view->DefineUserSymbol(COLocSym);
	m_view->DefineUserDataVariable(m_address, GetType());
	return COLocSym;
}

std::string CompleteObjectLocator::GetSymbolName()
{
	std::string symName = GetTypeDescriptor().GetDemangledName() + "::`RTTI Complete Object Locator'";
	if (auto subObjectTypeDesc = GetSubObjectTypeDescriptor())
	{
		symName = symName + "{for `" + subObjectTypeDesc->GetDemangledName() + "'}";
	}
	return symName;
}

std::string CompleteObjectLocator::GetClassName()
{
	return GetTypeDescriptor().GetDemangledName();
}