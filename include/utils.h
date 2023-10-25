#pragma once

#include <binaryninjaapi.h>
#include <string>

using namespace BinaryNinja;

uint64_t ReadIntWithSize(BinaryReader* reader, size_t size);
std::string IntToHex(uint64_t val);
Ref<TagType> GetConstructorTagType(BinaryView* view);
Ref<TagType> GetVirtualFunctionTableTagType(BinaryView* view);
Ref<TagType> GetVirtualFunctionTagType(BinaryView* view);
Ref<TagType> GetCOLocatorTagType(BinaryView* view);
Ref<Type> GetPointerTypeChildStructure(Ref<Type> ptrType);
std::optional<size_t> GetSSAVariableUnscopedDefinition(Ref<MediumLevelILFunction> mlil, SSAVariable var);
std::optional<size_t> WalkToSSAVariableOffset(Ref<MediumLevelILFunction> mlil, SSAVariable var);
std::vector<SSAVariable> GetSSAVariablesForVariable(Ref<MediumLevelILFunction> func, const Variable& var);
uint64_t ResolveRelPointer(BinaryView* view, uint64_t ptrVal);