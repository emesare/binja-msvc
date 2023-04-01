#pragma once

#include <binaryninjaapi.h>

using namespace BinaryNinja;

uint64_t ReadIntWithSize(BinaryReader* reader, size_t size);