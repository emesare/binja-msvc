#pragma once

#include <binaryninjaapi.h>

using namespace BinaryNinja;

class VirtualFunction
{
private:
	Ref<BinaryView> m_view;

public:
	uint64_t m_vftAddr;
	Ref<Function> m_func;

	VirtualFunction(BinaryView* view, uint64_t vftAddr, Ref<Function> func);
	// If the function is only referenced once.
	// NOTE: If you create for example, a vfunc that is able to be deduped, two vtables will point to that function, we
	// don't want to mislead by renaming the function to the last of those vtables associated class.
	bool IsUnique();
	Ref<Symbol> CreateSymbol(std::string name, std::string rawName);
};