#pragma once

#include <binaryninjaapi.h>
#include <string>

using namespace BinaryNinja;

uint64_t ReadIntWithSize(BinaryReader* reader, size_t size);
std::string IntToHex(uint64_t val);
Ref<TagType> GetConstructorTagType(BinaryView* view);
Ref<TagType> GetVirtualFunctionTableTagType(BinaryView* view);