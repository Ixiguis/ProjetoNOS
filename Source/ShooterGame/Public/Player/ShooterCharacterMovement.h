// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

/**
 * Movement component meant for use with Pawns.
 */

#pragma once
#include "ShooterCharacterMovement.generated.h"

UCLASS()
class SHOOTERGAME_API UShooterCharacterMovement : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UShooterCharacterMovement();

	virtual void InitializeComponent() override;

	virtual bool CanCrouchInCurrentState() const override;

	/** overriding to _add_ Z velocity, rather than _override_ it, to allow lift-jump */
	virtual bool DoJump(bool bReplayingMoves) override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	virtual void SetUpdatedComponent(USceneComponent* NewUpdatedComponent) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Movement)
	void SetSoulHunterSpeedMod(float NewMod);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Movement)
	void SetPowerupSpeedMod(float NewMod);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Movement)
	void SetEnvironmentSpeedMod(float NewMod);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Movement)
	void SetGameModeSpeedMod(float NewMod);

	UFUNCTION()
	void OnRep_SpeedModifier();

private:
	/** determines character velocity by checking location before and after Tick()
	*	because velocity is zero when moving on a base (e.g., a lift) */
	FVector CharacterVelocity;

	/** reference to the ShooterCharacter that uses this component */
	class AShooterCharacter* ShooterCharacterOwner;

	float DefaultMaxWalkSpeed, DefaultMaxAcceleration;

	UPROPERTY(ReplicatedUsing = OnRep_SpeedModifier)
	float SoulHunterSpeedMod;

	UPROPERTY(ReplicatedUsing = OnRep_SpeedModifier)
	float PowerupSpeedMod;

	UPROPERTY(ReplicatedUsing = OnRep_SpeedModifier)
	float EnvironmentSpeedMod;

	UPROPERTY(ReplicatedUsing = OnRep_SpeedModifier)
	float GameModeSpeedMod;
};
