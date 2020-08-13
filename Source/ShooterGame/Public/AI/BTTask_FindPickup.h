// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindPickup.generated.h"

// Bot AI Task that attempts to locate a pickup 
UCLASS()
class UBTTask_FindPickup : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_FindPickup() {}

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
