// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#pragma once

#include "GameRules/ShooterGameMode_TeamDeathMatch.h"
#include "ShooterGameMode_CTF.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERGAME_API AShooterGameMode_CTF : public AShooterGameMode_TeamDeathMatch
{
	GENERATED_BODY()

public:
	AShooterGameMode_CTF();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	virtual void InitGameState() override;
	
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category=GameMode)
	void BroadcastFlagTaken(uint8 FlagIdx, AShooterCharacter* Taker);


protected:

	void Score(AShooterCharacter* Scorer);

	TArray<class AShooterFlag*> Flags;

	void LoadFlags();
	
	void LoadFlags_Step2();

	virtual void Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, TSubclassOf<class AShooterWeapon> KillerWeaponClass, TSubclassOf<class UShooterDamageType> KillerDmgType) override;
	
	/** check who won. Focuses on the winner team's flag. */
	virtual class AActor* DetermineMatchWinner() override;
	
	/** return all flags when match ends */
	virtual void FinishMatch() override;

	virtual void CheckMatchEnd() override;

	/** When the flag has been dropped, it will automatically return to its base after this many seconds, if no one takes it. */
	UPROPERTY(Config)
	float FlagAutoReturnTime;
};
