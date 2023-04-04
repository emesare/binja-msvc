#include <binaryninjaapi.h>
#include <Demangle.h>

#include "object_locator.h"
#include "utils.h"

using namespace BinaryNinja;

TypeDescriptor::TypeDescriptor(BinaryView* view, uint64_t address)
{
	uint64_t addrSize = view->GetAddressSize();
	BinaryReader reader = BinaryReader(view);
	reader.Seek(address);

	m_view = view;
	m_address = address;
	m_pVFTableValue = ReadIntWithSize(&reader, addrSize);
	m_spareValue = ReadIntWithSize(&reader, addrSize);
	m_nameValue = reader.ReadCString(512);
}

std::string TypeDescriptor::GetDemangledName()
{
	std::string demangledNameValue =
		std::string(llvm::microsoftDemangle(m_nameValue.c_str(), nullptr, nullptr, nullptr, nullptr));

	size_t beginFind = demangledNameValue.find_first_of(" ");
	if (beginFind != std::string::npos)
	{
		demangledNameValue.erase(0, beginFind + 1);
	}
	size_t endFind = demangledNameValue.find(" `RTTI Type Descriptor Name'");
	if (endFind != std::string::npos)
	{
		demangledNameValue.erase(endFind, demangledNameValue.length());
	}

	return demangledNameValue;
}

Ref<Type> TypeDescriptor::GetType()
{
	size_t addrSize = m_view->GetAddressSize();

	StructureBuilder typeDescriptorBuilder;
	typeDescriptorBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType(), true), "pVFTable");
	typeDescriptorBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "spare");
	// Char array needs to be individually resized.
	typeDescriptorBuilder.AddMember(Type::ArrayType(Type::IntegerType(1, true, "char"), m_nameValue.length()), "name");

	return TypeBuilder::StructureType(&typeDescriptorBuilder).Finalize();
}

Ref<Symbol> TypeDescriptor::CreateSymbol(std::string name)
{
	Ref<Symbol> typeDescSym = new Symbol {DataSymbol, name, m_address};
	m_view->DefineUserSymbol(typeDescSym);
	m_view->DefineDataVariable(m_address, GetType());
	return typeDescSym;
}