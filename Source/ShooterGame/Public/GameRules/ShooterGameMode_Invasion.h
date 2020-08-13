// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#pragma once

#include "ShooterTypes.h"
#include "GameRules/ShooterGameState_Invasion.h"
#include "GameRules/ShooterGameMode_TeamDeathMatch.h"
#include "NavigationSystem.h"
#include "ShooterGameMode_Invasion.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERGAME_API AShooterGameMode_Invasion : public AShooterGameMode_TeamDeathMatch
{
	GENERATED_BODY()

public:
	AShooterGameMode_Invasion();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void InitBot(AShooterAIController* AIC, int32 BotNum) override;	
	virtual void InitGameState() override;
	virtual void HandleMatchHasStarted() override;
	FORCEINLINE FInvasionWave GetCurrWave() { return Waves[InvasionGameState->CurrentWave]; }
	
protected:

	AShooterGameState_Invasion* InvasionGameState;
	void SpawnMonster();
	void StartWave();
	void StopWave();
	bool GetSpawnPoint(FVector& OutSpawnLocation, AShooterCharacter* TestCharacter) const;

	virtual bool CanDealDamage(class AShooterPlayerState* DamageInstigator, class AShooterPlayerState* DamagedPlayer) const override;
	virtual void Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, TSubclassOf<class AShooterWeapon> KillerWeaponClass, TSubclassOf<class UShooterDamageType> KillerDmgType) override;
	virtual void DefaultTimer() override;
	virtual class AActor* DetermineMatchWinner() override;
	virtual void CheckMatchEnd() override;

	/** retrieved by the server from its PersistentUser */
	TArray<FInvasionWave> Waves;

	FTimerHandle SpawnMonsterHandle;
};
