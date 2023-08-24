﻿#pragma once
#include "PropertyDecorator.h"

const static TCHAR* StructPropDeco_PropPtrGroupStructTemp =
	LR"EOF(
struct {Declare_PropPtrGroupStructName}
{
  static int32 PropPointerMemOffsetCache[{Num_PropCount}]; 
  static bool bIsCacheInitialised; 
  {Declare_PropPtrGroupStructName}() {}
  {Declare_PropPtrGroupStructName}(void* Container)
  
  {
    auto ActorClass = {Declare_CppType}::StaticStruct();
    if (!ActorClass) return;
    {Code_AssignPropPointers}
    if (bIsCacheInitialised) {
      {Code_AssignPropertyPointers}
    }
    else {
     {Code_AssignPropertyPointersRuntime}
     bIsCacheInitialised = true;
    }
  }

  {Declare_PropPtrGroupStructName}(void* Params, FOutParmRec* OutParams)
  {
{Code_AssignPropPointersForRPC}
  }

{Declare_PropertyPointers}
  
  bool Merge(const {Declare_ProtoNamespace}::{Declare_ProtoStateMsgName}* FullState, {Declare_ProtoNamespace}::{Declare_ProtoStateMsgName}* DeltaState, UWorld* World)
  {
    bool bIsFullStateNull = FullState == nullptr;
    bool bStateChanged = false;
{Code_SetDeltaStates}
    return bStateChanged;
  }
  
  static bool Merge(void* Container, const {Declare_ProtoNamespace}::{Declare_ProtoStateMsgName}* FullState, {Declare_ProtoNamespace}::{Declare_ProtoStateMsgName}* DeltaState, UWorld* World, bool ForceMarge)
  {
    bool bIsFullStateNull = FullState == nullptr;
    bool bStateChanged = false;
{Code_StaticSetDeltaStates}
    return bStateChanged;
  }
  
  bool SetPropertyValue(const {Declare_ProtoNamespace}::{Declare_ProtoStateMsgName}* NewState, UWorld* World)
  {
    bool bStateChanged = false;
{Code_OnStateChange}
    return bStateChanged;
  }
  
  static bool SetPropertyValue(void* Container, const {Declare_ProtoNamespace}::{Declare_ProtoStateMsgName}* NewState, UWorld* World)
  {
    bool bStateChanged = false;
{Code_StaticOnStateChange}
    return bStateChanged;
  }

};

struct {Declare_PropCompilableStructName}
{
	{Code_StructCopyProperties}
};
)EOF";

const static TCHAR* StructPropDeco_AssignPropPtrTempRuntime =
    LR"EOF(
FString PropertyName = TEXT("{Declare_PropertyName}");
FProperty* Property = ActorClass->FindPropertyByName(FName(*PropertyName));
int32 Offset = Property->GetOffset_ForInternal();
PropPointerMemOffsetCache[{Num_PropIndex}] = Offset;
{Ref_AssignTo} = ({Declare_PropPtrGroupStructName})((uint8*){Ref_ContainerAddr} + Offset);
)EOF";

const static TCHAR* StructPropDeco_AssignPropPtrTemp =
    LR"EOF(
        {Ref_AssignTo} = ({Declare_PropPtrGroupStructName})(PropPointerMemOffsetCache[{Num_PropIndex}]);
)EOF";

const static TCHAR* StructPropDeco_SetDeltaStateArrayInnerTemp =
	LR"EOF(
{
  auto PropItem = &(*{Declare_PropertyPtr})[i];
  auto FullStateValue = i <  FullStateValueLength ? &{Declare_FullStateName}->{Definition_ProtoName}()[i] : nullptr;
  auto NewOne = {Declare_DeltaStateName}->add_{Definition_ProtoName}();
  bool bItemChanged = {Declare_PropPtrGroupStructName}::Merge(PropItem, FullStateValue, NewOne, {Code_GetWorldRef}, true);
  if (!bPropChanged && bItemChanged)
  {
  	bPropChanged = true;
  }
}
)EOF";

const static TCHAR* StructPropDeco_SetPropertyValueArrayInnerTemp =
	LR"EOF(
if ({Declare_PropPtrGroupStructName}::SetPropertyValue(&(*{Declare_PropertyPtr})[i], &MessageArr[i], {Code_GetWorldRef}) && !bPropChanged)
{
  bPropChanged = true;
}
)EOF";

class FStructPropertyDecorator : public FPropertyDecorator
{
public:

	FStructPropertyDecorator(FProperty* InProperty, IPropertyDecoratorOwner* InOwner);
	
	virtual void PostInit() override;
	
	virtual FString GetCompilableCPPType() override;

	virtual FString GetProtoFieldType() override;
	
	virtual FString GetProtoStateMessageType() override;
	
	virtual FString GetPropertyType() override;
	
	virtual FString GetDeclaration_PropertyPtr() override;
	
	virtual FString GetCode_AssignPropPointer(const FString& Container, const FString& AssignTo, int32 PropIndex) override;
	
	virtual TArray<FString> GetAdditionalIncludes() override;
	
	virtual FString GetCode_ActorPropEqualToProtoState(const FString& FromActor, const FString& FromState) override;
	virtual FString GetCode_ActorPropEqualToProtoState(const FString& FromActor, const FString& FromState, bool ForceFromPointer) override;
	
	virtual FString GetCode_SetDeltaState(const FString& TargetInstance, const FString& FullStateName, const FString& DeltaStateName, bool ConditionFullStateIsNull = false) override;

	virtual FString GetCode_SetDeltaStateByMemOffset(const FString& ContainerName, const FString& FullStateName, const FString& DeltaStateName, bool ConditionFullStateIsNull = false);

	virtual FString GetCode_SetDeltaStateArrayInner(const FString& TargetInstance, const FString& FullStateName, const FString& DeltaStateName, bool ConditionFullStateIsNull = false) override;

	virtual FString GetCode_SetPropertyValueTo(const FString& TargetInstance, const FString& NewStateName, const FString& AfterSetValueCode) override;

	virtual FString GetCode_OnStateChangeByMemOffset(const FString& ContainerName, const FString& NewStateName);

	virtual FString GetCode_SetPropertyValueArrayInner(const FString& TargetInstance, const FString& NewStateName) override;

	FString GetDeclaration_PropPtrGroupStruct();

	FString GetDeclaration_PropPtrGroupStructName();

	FString GetDefinition_ProtoStateMessage();

	virtual FString GetCode_GetWorldRef() override;

	virtual bool IsStruct() override;

	virtual TArray<TSharedPtr<FStructPropertyDecorator>> GetStructPropertyDecorators() override;
	
protected:
	TArray<TSharedPtr<FPropertyDecorator>> Properties;

	bool bIsBlueprintVariable;
};
