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