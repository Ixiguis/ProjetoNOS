// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AI/BTTask_FindPickup.h"
#include "GameRules/ShooterGameMode.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyAllTypes.h"
#include "Player/ShooterCharacter.h"
#include "AI/ShooterAIController.h"

EBTNodeResult::Type UBTTask_FindPickup::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	/*
	AShooterAIController* MyController = Cast<AShooterAIController>(OwnerComp.GetAIOwner());
	AShooterCharacter* MyBot = MyController ? Cast<AShooterCharacter>(MyController->GetPawn()) : NULL;
	if (MyBot == NULL)
	{
		return EBTNodeResult::Failed;
	}

	AShooterGameMode* GameMode = MyBot->GetWorld()->GetAuthGameMode<AShooterGameMode>();
	if (GameMode == NULL)
	{
		return EBTNodeResult::Failed;
	}

	const FVector MyLoc = MyBot->GetActorLocation();
	AShooterPickup_Ammo* BestPickup = NULL;
	float BestDistSq = MAX_FLT;

	for (int32 i = 0; i < GameMode->LevelPickups.Num(); ++i)
	{
		AShooterPickup_Ammo* AmmoPickup = Cast<AShooterPickup_Ammo>(GameMode->LevelPickups[i]);
		if (AmmoPickup && AmmoPickup->CanBePickedUp(MyBot))
		{
			const float DistSq = (AmmoPickup->GetActorLocation() - MyLoc).SizeSquared();
			if (BestDistSq == -1 || DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestPickup = AmmoPickup;
			}
		}
	}

	if (BestPickup)
	{
		OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID(), BestPickup->GetActorLocation());
		return EBTNodeResult::Succeeded;
	}
	*/
	return EBTNodeResult::Failed;
}
