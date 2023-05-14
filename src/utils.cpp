#include <binaryninjaapi.h>

using namespace BinaryNinja;

uint64_t ReadIntWithSize(BinaryReader* reader, size_t size)
{
	switch (size)
	{
	case 8:
		return reader->Read64();
	case 4:
		return reader->Read32();
	case 2:
		return reader->Read16();
	case 1:
		return reader->Read8();
	default:
		// TODO: Handle this correctly.
		return 0;
	}
}

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