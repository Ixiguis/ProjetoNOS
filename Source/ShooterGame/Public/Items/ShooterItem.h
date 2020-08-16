// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "ShooterItem.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable, NotPlaceable)
class AShooterItem : public AActor
{
	GENERATED_BODY()

public:
	AShooterItem();

	void SetOwningPawn(class AShooterCharacter* NewOwner);

	/** item enters MyPawn's inventory. Called automatically when MyPawn changes. */
	UFUNCTION(BlueprintImplementableEvent, Category=Item)
	void OnEnterInventory();

	/** item leaves MyPawn's inventory, is used/consumed/destroyed, or MyPawn dies */
	UFUNCTION(BlueprintNativeEvent, Category=Item)
	void OnLeaveInventory();
	
	/** Item pickup blueprint, used to spawn dropped item. If NULL, item cannot be dropped. */
	UPROPERTY(EditDefaultsOnly, Category=Config)
	TSubclassOf<class AShooterPickup> ItemPickup;

protected:

	UPROPERTY(BlueprintReadOnly, Category=Item, Transient, ReplicatedUsing=OnRep_MyPawn)
	class AShooterCharacter* MyPawn;
	
	UFUNCTION()
	void OnRep_MyPawn();
	
	/** Item's name */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Item)
	FText ItemName;
	
};
