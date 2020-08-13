// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once
#include "AIController.h"
#include "Player/ShooterControllerInterface.h"
#include "ShooterAIController.generated.h"

UCLASS(config=Game)
class AShooterAIController : public AAIController, public IShooterControllerInterface
{
	GENERATED_BODY()

private:
	UPROPERTY(transient)
	UBlackboardComponent* BlackboardComp;

	UPROPERTY(transient)
	class UBehaviorTreeComponent* BehaviorComp;
public:
	
	AShooterAIController();

	// Begin AController interface
	virtual void GameHasEnded(class AActor* EndGameFocus = NULL, bool bIsWinner = false) override;
	virtual void OnPossess(class APawn* InPawn) override;
	virtual void BeginInactiveState() override;
	// End APlayerController interface
	
	void Respawn();

	void CheckAmmo(const class AShooterWeapon* CurrentWeapon);

	void SetEnemy(class APawn* InPawn);

	class AShooterCharacter* GetEnemy() const;

	/* If there is line of sight to current enemy, start firing at them */
	UFUNCTION(BlueprintCallable, Category=Behavior)
	void ShootEnemy();

	/* Finds the closest enemy and sets them as current target */
	UFUNCTION(BlueprintCallable, Category=Behavior)
	void FindClosestEnemy();
	
	UFUNCTION(BlueprintCallable, Category = Behavior)
	bool FindClosestEnemyWithLOS(AShooterCharacter* ExcludeEnemy);
		
	bool HasWeaponLOSToEnemy(AActor* InEnemyActor, const bool bAnyEnemy) const;

	/** Update direction AI is looking based on FocalPoint */
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;
	
	// Begin IShooterControllerInterface
	virtual bool IsEnemyFor(AController* TestPC) const;
	// End IShooterControllerInterface

protected:
	// Check of we have LOS to a character
	bool LOSTrace(AShooterCharacter* InEnemyChar) const;


	AShooterWeapon* SwitchToBestWeapon();
	AShooterWeapon* CurrentWeapon;
	float LastWeaponSwitchTime;
	int32 EnemyKeyID;
	int32 NeedAmmoKeyID;
	int32 bShouldRespawn : 1;
	
	/** Handle for efficient management of Respawn timer */
	FTimerHandle TimerHandle_Respawn;

public:
	/** Returns BlackboardComp subobject **/
	FORCEINLINE UBlackboardComponent* GetBlackboardComp() const { return BlackboardComp; }
	/** Returns BehaviorComp subobject **/
	FORCEINLINE UBehaviorTreeComponent* GetBehaviorComp() const { return BehaviorComp; }

};
