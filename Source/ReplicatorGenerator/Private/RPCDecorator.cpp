#include "RPCDecorator.h"

#include "PropertyDecoratorFactory.h"
#include "ReplicatedActorDecorator.h"
#include "ReplicatorGeneratorUtils.h"

FRPCDecorator::FRPCDecorator(UFunction* InFunc, IPropertyDecoratorOwner* InOwner)
	: FStructPropertyDecorator(nullptr, InOwner), OriginalFunction(InFunc)
{
	if (OriginalFunction != nullptr)
	{
		FPropertyDecoratorFactory& PropertyDecoratorFactory = FPropertyDecoratorFactory::Get();
		for (TFieldIterator<FProperty> SIt(OriginalFunction, EFieldIteratorFlags::ExcludeSuper); SIt; ++SIt)
		{
			TSharedPtr<FPropertyDecorator> PropertyDecoratorPtr = PropertyDecoratorFactory.GetPropertyDecorator(*SIt, this);
			if (PropertyDecoratorPtr.IsValid())
			{
				PropertyDecoratorPtr->SetForceNotDirectlyAccessible(true);
				Properties.Add(PropertyDecoratorPtr);
			}
		}
	}
	OwnerActor = static_cast<FReplicatedActorDecorator*>(InOwner);
}

bool FRPCDecorator::Init(const TFunction<FString()>& SetNameForIllegalPropName)
{
	if (bInitialized)
	{
		return false;
	}

	CompilablePropName = OriginalFunction->GetName();
	if (ChanneldReplicatorGeneratorUtils::ContainsUncompilableChar(CompilablePropName))
	{
		CompilablePropName = *SetNameForIllegalPropName();
	}
	// The generated params struct name will be added to 'ChanneldGlobalStruct.h'.
	// Add ActorNameHash to avoid name conflict.
	CompilablePropName = FString::Printf(TEXT("%s_%s"), *CompilablePropName, *ChanneldReplicatorGeneratorUtils::GetHashString(OwnerActor->GetActorPathName()));

	PostInit();
	bInitialized = true;
	return true;
}

bool FRPCDecorator::IsDirectlyAccessible()
{
	return false;
}

FString FRPCDecorator::GetCPPType()
{
	return FString::Printf(TEXT("RPCParamStruct%s"), *GetPropertyName());
}

FString FRPCDecorator::GetProtoStateMessageType()
{
	return FString::Printf(TEXT("RPCParams%s"), *GetPropertyName());
}

FString FRPCDecorator::GetCode_SerializeFunctionParams()
{
	FStringFormatNamedArguments FormatArgs;
	FormatArgs.Add(TEXT("Declare_FuncName"), OriginalFunction->GetName());
	FormatArgs.Add(TEXT("Declare_PropPtrGroupStructName"), GetDeclaration_PropPtrGroupStructName());
	FormatArgs.Add(TEXT("Declare_ProtoNamespace"), GetProtoNamespace());
	FormatArgs.Add(TEXT("Declare_ProtoStateMsgName"), GetProtoStateMessageType());
	FormatArgs.Add(TEXT("Code_GetWorldRef"), Owner->GetCode_GetWorldRef());
	return FString::Format(RPC_SerializeFuncParamsTemp, FormatArgs);
}

FString FRPCDecorator::GetCode_DeserializeFunctionParams()
{
	FStringFormatNamedArguments FormatArgs;
	FormatArgs.Add(TEXT("Declare_FuncName"), OriginalFunction->GetName());
	FormatArgs.Add(TEXT("Declare_PropPtrGroupStructName"), GetDeclaration_PropPtrGroupStructName());
	FormatArgs.Add(TEXT("Declare_ProtoNamespace"), GetProtoNamespace());
	FormatArgs.Add(TEXT("Declare_ProtoStateMsgName"), GetProtoStateMessageType());
	FormatArgs.Add(TEXT("Declare_ParamStructCopy"), GetCompilableCPPType());
	FormatArgs.Add(TEXT("Code_GetWorldRef"), Owner->GetCode_GetWorldRef());
	return FString::Format(RPC_DeserializeFuncParamsTemp, FormatArgs);
}

FString FRPCDecorator::GetDeclaration_PropPtrGroupStruct()
{
    FStringFormatNamedArguments FormatArgs;
    FormatArgs.Add(TEXT("Declare_PropPtrGroupStructName"), GetDeclaration_PropPtrGroupStructName());
    FString AssignPropertyPointerCodes;
    FString AssignPropPointersForRPCCodes;
    FString DeclarePropPtrCodes;
    FString SetDeltaStateCodes;
    FString StaticSetDeltaStateCodes;
    FString OnStateChangeCodes;
    FString StaticOnStateChangeCodes;
    FString StructCopyCode;
    int32 PrevPropMemEnd = 0;

    bool bFirstProperty = true;
    int32 i = 0;
    for (TSharedPtr<FPropertyDecorator> PropDecorator : Properties)
    {
        DeclarePropPtrCodes.Append(FString::Printf(TEXT("%s;\n"), *PropDecorator->GetDeclaration_PropertyPtr()));

        AssignPropertyPointerCodes.Append(
            FString::Printf(
                TEXT("{\n%s;\n}\n"),
                *PropDecorator->GetCode_AssignPropPointerForRPC(TEXT("Container"), PropDecorator->GetPointerName())
            )
        );

        if (PropDecorator->HasAnyPropertyFlags(CPF_OutParm))
        {
            AssignPropPointersForRPCCodes.Append(
                FString::Printf(
                    TEXT("{\n%s;\nOutParams = OutParams->NextOutParm;\n}\n"),
                    *PropDecorator->GetCode_AssignPropPointerForRPC(TEXT("OutParams->PropAddr"), PropDecorator->GetPointerName())
                )
            );
        }
        else
        {
            AssignPropPointersForRPCCodes.Append(
                FString::Printf(
                    TEXT("{\n%s;\n}\n"),
                    *PropDecorator->GetCode_AssignPropPointerForRPC(TEXT("Params"), PropDecorator->GetPointerName())
                )
            );
        }

        SetDeltaStateCodes.Append(
            PropDecorator->GetCode_SetDeltaState(
                TEXT("this"), TEXT("FullState"), TEXT("DeltaState"), true
            )
        );
        StaticSetDeltaStateCodes.Append(
            PropDecorator->GetCode_SetDeltaStateByMemOffsetForRPC(
                TEXT("Container"), TEXT("FullState"), TEXT("DeltaState"), true
            )
        );

        OnStateChangeCodes.Append(
            PropDecorator->GetCode_OnStateChange(
                TEXT("this"), TEXT("NewState")
            )
        );

        StaticOnStateChangeCodes.Append(
            PropDecorator->GetCode_OnStateChangeByMemOffsetForRPC(
                TEXT("Container"), TEXT("NewState")
            )
        );

        StructCopyCode.Append(
            FString::Printf(TEXT("%s %s;\n"), *PropDecorator->GetCompilableCPPType(), *PropDecorator->GetPropertyName())
        );
        if (bFirstProperty) { bFirstProperty = false; }
        i++;
    }
    FormatArgs.Add(TEXT("Code_AssignPropPointers"), *AssignPropertyPointerCodes);
    FormatArgs.Add(TEXT("Code_AssignPropPointersForRPC"), *AssignPropPointersForRPCCodes);
    FormatArgs.Add(TEXT("Declare_PropertyPointers"), *DeclarePropPtrCodes);
    FormatArgs.Add(TEXT("Declare_ProtoNamespace"), GetProtoNamespace());
    FormatArgs.Add(TEXT("Declare_ProtoStateMsgName"), GetProtoStateMessageType());
    FormatArgs.Add(TEXT("Code_SetDeltaStates"), SetDeltaStateCodes);
    FormatArgs.Add(TEXT("Code_StaticSetDeltaStates"), StaticSetDeltaStateCodes);
    FormatArgs.Add(TEXT("Code_OnStateChange"), OnStateChangeCodes);
    FormatArgs.Add(TEXT("Code_StaticOnStateChange"), StaticOnStateChangeCodes);

    FormatArgs.Add(TEXT("Declare_PropCompilableStructName"), GetCompilableCPPType());
    FormatArgs.Add(TEXT("Code_StructCopyProperties"), StructCopyCode);
    FormatArgs.Add(TEXT("Num_PropCount"), Properties.Num());
    FormatArgs.Add(TEXT("Declare_CppType"), GetCPPType());
    return FString::Format(RPCPropDeco_PropPtrGroupStructTemp, FormatArgs);
}

FString FRPCDecorator::GetCode_AssignPropPointer(const FString& Container, const FString& AssignTo)
{
    return GetCode_AssignPropPointer(Container, AssignTo, GetMemOffset());
}

FString FRPCDecorator::GetCode_AssignPropPointer(const FString& Container, const FString& AssignTo, int32 MemOffset)
{
    FStringFormatNamedArguments FormatArgs;
    FormatArgs.Add(TEXT("Ref_AssignTo"), AssignTo);
    FormatArgs.Add(TEXT("Ref_ContainerAddr"), Container);
    FormatArgs.Add(TEXT("Num_PropMemOffset"), MemOffset);
    FormatArgs.Add(TEXT("Declare_PropPtrGroupStructName"), GetDeclaration_PropPtrGroupStructName());
    FormatArgs.Add(TEXT("Declare_PropertyName"), GetPropertyName());

    return FString::Format(RPCPropDeco_AssignPropPtrTemp, FormatArgs);
}

FString FRPCDecorator::GetCode_GetWorldRef()
{
	return TEXT("World");
}
