#pragma once
#include "PropertyDecorator.h"
#include "PropertyDecorator/StructPropertyDecorator.h"

const static TCHAR* RPCPropDeco_PropPtrGroupStructTemp =
LR"EOF(
struct {Declare_PropPtrGroupStructName}
{
  {Declare_PropPtrGroupStructName}() {}
  {Declare_PropPtrGroupStructName}(void* Container)
  
  {
    {Code_AssignPropPointers}
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

const static TCHAR* RPCPropDeco_AssignPropPtrTemp =
LR"EOF(
void* PropertyAddr = (uint8*){Ref_ContainerAddr} + {Num_PropMemOffset};
{Ref_AssignTo} = {Declare_PropPtrGroupStructName}(PropertyAddr));
)EOF";

static const TCHAR* RPC_SerializeFuncParamsTemp =
	LR"EOF(
if(Func->GetFName() == FName("{Declare_FuncName}"))
{
  {Declare_PropPtrGroupStructName} ParamPointerGroup(Params, OutParams);
  auto Msg = new {Declare_ProtoNamespace}::{Declare_ProtoStateMsgName}();
  ParamPointerGroup.Merge(nullptr, Msg, {Code_GetWorldRef});
  return MakeShareable(Msg);
}
)EOF";

static const TCHAR* RPC_DeserializeFuncParamsTemp =
	LR"EOF(
if(Func->GetFName() == FName("{Declare_FuncName}"))
{
  {Declare_ProtoNamespace}::{Declare_ProtoStateMsgName} Msg;
  if (!Msg.ParseFromString(ParamsPayload))
  {
    UE_LOG(LogChanneldGen, Warning, TEXT("Failed to parse {Declare_FuncName} Params"));
    return nullptr;
  }
  {Declare_ParamStructCopy}* Params = new {Declare_ParamStructCopy}();
  {Declare_PropPtrGroupStructName}::SetPropertyValue(Params, &Msg, {Code_GetWorldRef});

  return MakeShareable(Params);
}
)EOF";

class FRPCDecorator : public FStructPropertyDecorator
{
public:
	FRPCDecorator(UFunction*, IPropertyDecoratorOwner*);

	virtual ~FRPCDecorator() = default;

	virtual bool Init(const TFunction<FString()>& SetNameForIllegalPropName) override;
	
	virtual bool IsDirectlyAccessible() override;

	virtual FString GetCPPType() override;
	
	virtual FString GetProtoStateMessageType() override;

	FString GetCode_SerializeFunctionParams();
	FString GetCode_DeserializeFunctionParams();

    virtual FString GetDeclaration_PropPtrGroupStruct() override;
    //Wrapper Function to call GetMemOffSet()
    FString GetCode_AssignPropPointer(const FString& Container, const FString& AssignTo);
    virtual FString GetCode_AssignPropPointer(const FString& Container, const FString& AssignTo, int32 MemOffset) override;
	
     //FString GetCode_OnStateChangeByMemOffset(const FString& ContainerName, const FString& NewStateName, int32 MemOffset) override;
     //FString GetCode_SetDeltaStateByMemOffset(const FString& ContainerName, const FString& FullStateName, const FString& DeltaStateName, int32 MemOffset, bool ConditionFullStateIsNull = false) override;
	virtual FString GetCode_GetWorldRef() override;

protected:
	FReplicatedActorDecorator* OwnerActor;
	UFunction* OriginalFunction;
	FName FunctionName;
};
