#include <binaryninjaapi.h>
#include <mediumlevelilinstruction.h>

using namespace BinaryNinja;

std::string IntToHex(uint64_t val)
{
	std::stringstream stream;
	stream << std::hex << val;
	return stream.str();
}

Ref<TagType> GetConstructorTagType(BinaryView* view)
{
	Ref<TagType> tagType = view->GetTagType("MSVC Constructor");
	if (tagType == nullptr)
	{
		tagType = new TagType(view, "MSVC Constructor", "👷");
		view->AddTagType(tagType);
	}
	return tagType;
}

Ref<TagType> GetVirtualFunctionTableTagType(BinaryView* view)
{
	Ref<TagType> tagType = view->GetTagType("MSVC Virtual Function Table");
	if (tagType == nullptr)
	{
		tagType = new TagType(view, "MSVC Virtual Function Table", "🗞");
		view->AddTagType(tagType);
	}
	return tagType;
}

Ref<TagType> GetVirtualFunctionTagType(BinaryView* view)
{
	Ref<TagType> tagType = view->GetTagType("MSVC Virtual Function");
	if (tagType == nullptr)
	{
		tagType = new TagType(view, "MSVC Virtual Function", "☝️");
		view->AddTagType(tagType);
	}
	return tagType;
}

Ref<TagType> GetCOLocatorTagType(BinaryView* view)
{
	Ref<TagType> tagType = view->GetTagType("MSVC Complete Object Locator");
	if (tagType == nullptr)
	{
		tagType = new TagType(view, "MSVC Complete Object Locator", "✨");
		view->AddTagType(tagType);
	}
	return tagType;
}

Ref<Type> GetPointerTypeChildStructure(Ref<Type> ptrType)
{
	if (!ptrType->IsPointer())
		return 0;
	Ref<Type> childType = ptrType->GetChildType().GetValue();
	while (childType->IsPointer())
	{
		childType = childType->GetChildType().GetValue();
	}
	return childType;
}

uint64_t ResolveRelPointer(BinaryView* view, uint64_t ptrVal)
{
	switch (view->GetAddressSize())
	{
	case 8:
		return view->GetStart() + ptrVal;
	case 4:
		return ptrVal;
	default:
		// TODO: Handle this correctly.
		return 0;
	}
}

// void GetAllMembersForStructure(BinaryView* view, Ref<Function> func, StructureBuilder structBuilder, Variable var)
// {
// 	auto mlil = func->GetMediumLevelIL();
// 	for (auto varRef : func->GetMediumLevelILVariableReferences(var))
// 	{
// 		auto inst = mlil[varRef.exprId];
// 	}

// 	// TODO: Scan the function for accesses to the var, make sure this works correctly with inherited classes, i.e. make
// 	// sure they dont already exist.
// }