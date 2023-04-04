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

std::vector<VirtualFunction> CompleteObjectLocator::GetVirtualFunctions()
{
	size_t addrSize = m_view->GetAddressSize();
	std::vector<VirtualFunction> vFuncs = {};
	BinaryReader reader = BinaryReader(m_view);

	std::vector<uint64_t> objLocatorRefs = m_view->GetDataReferences(m_address);
	if (objLocatorRefs.empty())
	{
		LogWarn("CompleteObjectLocator at %x has no data references!", m_address);
		return vFuncs;
	}

	uint64_t vftAddr = objLocatorRefs.front() + addrSize;
	reader.Seek(vftAddr);

	while (true)
	{
		uint64_t vFuncAddr = ReadIntWithSize(&reader, addrSize);
		auto funcs = m_view->GetAnalysisFunctionsForAddress(vFuncAddr);
		if (funcs.empty())
		{
			Ref<Segment> segment = m_view->GetSegmentAt(vFuncAddr);
			if (segment == nullptr)
			{
				// Last CompleteObjectLocator?
				break;
			}

			if (segment->GetFlags() & (SegmentExecutable | SegmentDenyWrite))
			{
				LogInfo("Discovered function from vtable reference -> %x", vFuncAddr);
				m_view->CreateUserFunction(m_view->GetDefaultPlatform(), vFuncAddr);
				funcs = m_view->GetAnalysisFunctionsForAddress(vFuncAddr);
			}
			else
			{
				// Hit the next CompleteObjectLocator
				break;
			}
		}

		vFuncs.emplace_back(VirtualFunction(m_view, vFuncAddr, funcs.front()));
	}


	return vFuncs;
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
	Ref<Symbol> objLocatorSym = new Symbol {DataSymbol, name, m_address};
	return m_view->DefineAutoSymbolAndVariableOrFunction(
		m_view->GetDefaultPlatform(), objLocatorSym, GetCompleteObjectLocatorType(m_view));
}