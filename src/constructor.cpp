#include <binaryninjaapi.h>
#include <mediumlevelilinstruction.h>

#include "constructor.h"

using namespace BinaryNinja;

Constructor::Constructor(BinaryView* view, Function* func)
{
	m_view = view;
	m_func = func;
}

bool Constructor::IsValid()
{
	if (GetRootVirtualFunctionTable().has_value())
	{
		// Check to make sure we have the first param as a _vfTable** type...
		auto paramVars = m_func->GetParameterVariables();
		if (!paramVars->empty())
		{
			auto registeredName = m_func->GetVariableType(paramVars->front())->GetRegisteredName();
			if (registeredName == nullptr
				&& registeredName->GetName().GetString().find("_vfTable") != std::string::npos)
				return true;
		}
	}

	return false;
}

std::string Constructor::GetName()
{
	return GetRootVirtualFunctionTable().value().GetCOLocator().GetTypeDescriptor().GetDemangledName();
}

std::string Constructor::GetRawName()
{
	return GetRootVirtualFunctionTable().value().GetCOLocator().GetUniqueName();
}

std::vector<Constructor> Constructor::GetInnerConstructors()
{
	std::vector<Constructor> innerConstructors = {};

	for (auto callSite : m_func->GetCallSites())
	{
		for (auto calleeAddr : m_view->GetCallees(callSite))
		{
			auto calleeFuncs = m_view->GetAnalysisFunctionsForAddress(calleeAddr);
			if (calleeFuncs.empty())
				continue;

			for (auto calleeFunc : calleeFuncs)
			{
				// TODO: Get rid of second constructor creation
				if (Constructor(m_view, calleeFunc).IsValid())
				{
					innerConstructors.emplace_back(Constructor(m_view, calleeFunc));
				}
			}
		}
	}

	return innerConstructors;
}

std::optional<VirtualFunctionTable> Constructor::GetRootVirtualFunctionTable()
{
	Ref<MediumLevelILFunction> mlil = m_func->GetMediumLevelIL();
	if (mlil == nullptr)
		return std::nullopt;
	Ref<MediumLevelILFunction> mlilssa = mlil->GetSSAForm();
	if (mlilssa == nullptr)
		return std::nullopt;
	std::optional<uint64_t> rootVftAddr = std::nullopt;

	// TODO: Shouldn't we make sure that the vfTable is assigned to the return value?
	for (auto& block : mlilssa->GetBasicBlocks())
	{
		for (size_t instIdx = block->GetStart(); instIdx < block->GetEnd(); instIdx++)
		{
			MediumLevelILInstruction inst = (*mlilssa)[instIdx];

			inst.VisitExprs([&](const MediumLevelILInstruction& expr) {
				switch (expr.operation)
				{
				case MLIL_STORE_SSA:
					auto destExpr = expr.GetDestExpr<MLIL_STORE_SSA>();
					auto sourceExpr = expr.GetSourceExpr<MLIL_STORE_SSA>();
					DataVariable sourceDataVar = {};

					if (m_view->GetDataVariableAtAddress(sourceExpr.GetValue().value, sourceDataVar))
					{
						auto registeredName = sourceDataVar.type->GetRegisteredName();
						if (registeredName == nullptr)
							return true;
						// TODO: Check here to make sure we are setting constructorObjVar for a var that comes
						// from the first param.
						if (registeredName->GetName().GetString().find("_vfTable") != std::string::npos)
						{
							switch (destExpr.operation)
							{
							case MLIL_VAR_SSA:
								rootVftAddr = sourceDataVar.address;
								// Stop visiting.
								return false;
							}
						}
					}
				}
				return true;
			});

			// TODO: This is just awful.
			if (rootVftAddr.has_value())
				break;
		}

		// TODO: This is just awful.
		if (rootVftAddr.has_value())
			break;
	}

	if (rootVftAddr.has_value())
	{
		return VirtualFunctionTable(m_view, rootVftAddr.value());
	}
	else
	{
		return std::nullopt;
	}
}

// TODO: We need to check for heap allocated constructors, we should first identify all constructors with a
// reference to a vfTable then once we do that a second pass should check all callers of those constructor functions and
// see if they call `operator new` with the returned pointer being used as arg1 of those constructors.

// TODO: OR we could just get the return var and create fields for every access...

/*
void* rax = operator new(0x18)
struct SomeClass_vfTable** var_20
if (rax == 0)
    var_20 = nullptr
else
    __builtin_memset(s: rax, c: 0, n: 0x18)
    var_20 = std::_Future_error_category::_Future_error_category(rax)
    // array accesses after this are fields we need to populate!
*/

Ref<Type> Constructor::CreateObjectType()
{
	std::string idName = GetRawName();
	Ref<Type> typeCache = m_view->GetTypeById("msvc_" + idName);

	if (typeCache == nullptr)
	{
		StructureBuilder objBuilder = {};
		objBuilder.SetPropagateDataVariableReferences(true);
		// Collect all object fields.
		Ref<MediumLevelILFunction> mlil = m_func->GetMediumLevelIL()->GetSSAForm();
		for (auto& block : mlil->GetBasicBlocks())
		{
			for (size_t instIdx = block->GetStart(); instIdx < block->GetEnd(); instIdx++)
			{
				MediumLevelILInstruction inst = (*mlil)[instIdx];

				std::optional<SSAVariable> constructorObjVar = {};
				inst.VisitExprs([&](const MediumLevelILInstruction& expr) {
					switch (expr.operation)
					{
					case MLIL_STORE_SSA:
						auto destExpr = expr.GetDestExpr<MLIL_STORE_SSA>();
						int offset = 0;
						SSAVariable assignedVar = {};
						switch (destExpr.operation)
						{
						case MLIL_VAR_SSA:
							// NOTE: Constructor obj must give a 0 offset assignment in its constructor func.
							assignedVar = destExpr.GetRawOperandAsSSAVariable(0);
							break;
						case MLIL_ADD:
							auto leftExpr = destExpr.GetLeftExpr<MLIL_ADD>();
							auto rightExpr = destExpr.GetRightExpr<MLIL_ADD>();
							assignedVar = leftExpr.GetRawOperandAsSSAVariable(0);
							offset = rightExpr.GetValue().value;
							break;
						};

						auto sourceExpr = expr.GetSourceExpr<MLIL_STORE_SSA>();
						DataVariable sourceDataVar = {};

						if (m_view->GetDataVariableAtAddress(sourceExpr.GetValue().value, sourceDataVar))
						{
							auto registeredName = sourceDataVar.type->GetRegisteredName();
							if (registeredName == nullptr)
								return true;
							if (registeredName->GetName().GetString().find("_vfTable") != std::string::npos)
							{
								// TODO: Check here to make sure we are setting constructorObjVar for a var that comes
								// from the first param.
								switch (destExpr.operation)
								{
								case MLIL_VAR_SSA:
									constructorObjVar = destExpr.GetRawOperandAsSSAVariable(0);
									break;
								case MLIL_ADD:
									auto leftExpr = destExpr.GetLeftExpr<MLIL_ADD>();
									constructorObjVar = leftExpr.GetRawOperandAsSSAVariable(0);
									break;
								}

								VirtualFunctionTable currentVft = VirtualFunctionTable(m_view, sourceDataVar.address);
								CompleteObjectLocator currentVftCOLocator = currentVft.GetCOLocator();
								std::string currentVftName = currentVftCOLocator.GetUniqueName();
								if (currentVftCOLocator.IsSubObject())
									currentVftName.erase(0, 2);

								objBuilder.AddMemberAtOffset(
									Type::PointerType(m_view->GetAddressSize(),
										m_view->GetTypeByRef(sourceDataVar.type->GetRegisteredName())),
									"vft_" + currentVftName, offset);
							}
							else if (constructorObjVar.has_value() && assignedVar == constructorObjVar.value())
							{
								// TODO: Check to see if destExpr uses the `constructorObjVar` (TODO: Dont we do that?)
								objBuilder.AddMemberAtOffset(
									sourceDataVar.type, "field_" + std::to_string(offset), offset);
							}
						}
					}
					return true;
				});
			}
		}

		std::vector<BaseStructure> innerStructures = {};
		for (auto& innerConstructor : GetInnerConstructors())
		{
			// TODO: What happens if a field exists in an inherited class and the root class?
			LogDebug("inner -> %s", innerConstructor.GetName().c_str());
			innerStructures.emplace_back(BaseStructure(innerConstructor.CreateObjectType(), 0));
		}
		objBuilder.SetBaseStructures(innerStructures);

		m_view->DefineType(
			"msvc_" + idName, QualifiedName(GetName()), TypeBuilder::StructureType(&objBuilder).Finalize());

		typeCache = m_view->GetTypeById("msvc_" + idName);
	}

	return typeCache;
}

Ref<Symbol> Constructor::CreateSymbol()
{
	std::string symName = GetName();
	std::string symRawName = GetRawName();
	std::string funcName = symName + "::" + symName;
	Ref<Symbol> newFuncSym =
		new Symbol {FunctionSymbol, funcName, funcName, symRawName + "::" + symRawName, m_func->GetStart()};
	m_view->DefineUserSymbol(newFuncSym);
	return newFuncSym;
}