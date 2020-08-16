// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#pragma once

#include "ShooterTypes.h"
#include "GameRules/ShooterGameState_Invasion.h"
#include "GameRules/ShooterGameMode_TeamDeathMatch.h"
#include "NavigationSystem.h"
#include "ShooterGameMode_Invasion.generated.h"


USTRUCT(BlueprintType)
struct FInvasionMonster
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InvasionMonster)
	TSubclassOf<AShooterCharacter> PawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InvasionMonster)
	int32 HealthOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InvasionMonster)
	float MaxWalkSpeedOverride;

	FInvasionMonster()
	{
		HealthOverride = -1;
		MaxWalkSpeedOverride = -1.f;
	}
	FInvasionMonster(TSubclassOf<AShooterCharacter> InPawnClass, int32 InHealthOverride = -1, float InMaxWalkSpeedOverride = -1.f)
	{
		PawnClass = InPawnClass;
		HealthOverride = InHealthOverride;
		MaxWalkSpeedOverride = InMaxWalkSpeedOverride;
	}
};

USTRUCT(BlueprintType)
struct FInvasionWave
{
	GENERATED_USTRUCT_BODY()

	/** when a monster is spawned, it will be picked randomly from this array */
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InvasionWave)
		TArray<FInvasionMonster> InvasionMonsters;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InvasionWave)
	int32 MaxMonsters;

	/** wave duration, in seconds. Stops spawning more monsters if the duration was reached (even if MaxMonsters wasn't reached). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InvasionWave)
	float WaveDuration;

	/** how many monsters will spawn per minute (up to MaxMonsters) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InvasionWave)
	float MonstersSpawnRate;

	/** how long players have to prepare themselves before monsters start spawning. In seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InvasionWave)
	float WarmupTime;

	FInvasionWave(int32 InMaxMonsters = 50, float InWaveDuration = 240.f, float InWarmupTime = 10.f, float InMonstersSpawnRate = 20.f)
	{
		MaxMonsters = InMaxMonsters;
		WaveDuration = InWaveDuration;
		WarmupTime = InWarmupTime;
		MonstersSpawnRate = InMonstersSpawnRate;
	}
	void AddInvasionMonster(FInvasionMonster Monster)
	{
		InvasionMonsters.Add(Monster);
	}
};


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
