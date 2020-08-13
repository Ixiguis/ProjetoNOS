// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTree/BTNode.h"
#include "BTTask_FindPointNearEnemy.generated.h"

// Bot AI task that tries to find a location near the current enemy
UCLASS()
class UBTTask_FindPointNearEnemy : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_FindPointNearEnemy() {}

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
