// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#pragma once

#include "GameFramework/Actor.h"
#include "ShooterFlag.generated.h"

/**
 * A flag, in CTF game. This is spawned by the server at game start, do not place it manually on the map.
 * Place a FlagBase instead.
 */
UCLASS(Abstract, Blueprintable, NotPlaceable)
class SHOOTERGAME_API AShooterFlag : public AActor
{
	GENERATED_BODY()

public:

	AShooterFlag();

	/** returns true if the flag is at its respective base, false if it's dropped or taken by an enemy */
	UFUNCTION(BlueprintCallable, Category = Flag)
	bool IsAtBase() const;

	/** Taker takes the flag, and attaches the flag to Taker. Returns true if the flag was successfully taken. */
	bool TakeFlag(class AShooterCharacter* Taker);

	void ReturnFlag();
	void DropFlag();
	
	void SetFlagBase(class AShooterFlagBase* TheBase);

	class AShooterCharacter* GetFlagCarrier() const;

	/** if true then this flag was dropped, otherwise it's at base (if FlagCarrier == NULL) or carried by someone */
	UPROPERTY(Replicated)
	bool IsDropped;
	
	/** When the flag has been dropped, it will automatically return to its base after this many seconds, if no one takes it. */
	UPROPERTY(Transient, Replicated)
	float AutoReturnTime;

protected:
	/** To which team this flag belongs */
	UPROPERTY(BlueprintReadOnly, Replicated, Category=Flag)
	uint8 TeamNumber;
	
	UPROPERTY(Replicated)
	class AShooterCharacter* FlagCarrier;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category=Flag)
	class UCapsuleComponent* CollisionComp;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category=Flag)
	class UStaticMeshComponent* MeshComp;

	/** this flag's base */
	class AShooterFlagBase* MyFlagBase;

	float LastReturnTime;

	FTimerHandle ReturnFlagHandle;
};
