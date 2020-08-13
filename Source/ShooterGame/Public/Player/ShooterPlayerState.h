// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once

#include "ShooterPlayerState.generated.h"

UCLASS(config=Game)
class AShooterPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AShooterPlayerState();

	// Begin APlayerState interface
	/** clear scores */
	virtual void Reset() override;

	/**
	 * Set the team 
	 *
	 * @param	InController	The controller to initialize state with
	 */
	virtual void ClientInitialize(class AController* InController) override;
	virtual void UnregisterPlayerWithSession() override;

	// End APlayerState interface

	/**
	 * Set new team and update pawn. Also updates player character team colors.
	 *
	 * @param	NewTeamNumber	Team we want to be on.
	 */
	UFUNCTION(BlueprintCallable, Reliable, WithValidation, Server, Category=PlayerState)
	void ServerSetTeamNum(int32 NewTeamNumber);

	/** player killed someone */
	UFUNCTION(BlueprintCallable, Category=PlayerState)
	void ScoreKill(AShooterPlayerState* Victim, int32 Points);

	/** player died */
	UFUNCTION(BlueprintCallable, Category=PlayerState)
	void ScoreDeath(AShooterPlayerState* KilledBy, int32 Points);
	
	/** player suicided */
	UFUNCTION(BlueprintCallable, Category=PlayerState)
	void ScoreSuicide(int32 Points);
	
	/** get current team */
	UFUNCTION(BlueprintCallable, Category=PlayerState)
	int32 GetTeamNum() const;

	/** get number of kills */
	UFUNCTION(BlueprintCallable, Category=PlayerState)
	int32 GetKills() const;

	/** get number of deaths */
	UFUNCTION(BlueprintCallable, Category=PlayerState)
	int32 GetDeaths() const;
	
	/** get number of deaths */
	UFUNCTION(BlueprintCallable, Category=PlayerState)
	int32 GetSuicides() const;

	/** get number of bullets fired this match */
	int32 GetNumBulletsFired() const;

	/** get number of rockets fired this match */
	int32 GetNumRocketsFired() const;
	
	/** get whether the player quit the match */
	UFUNCTION(BlueprintCallable, Category=PlayerState)
	bool IsQuitter() const;

	/** gets truncated player name to fit in death log and scoreboards */
	UFUNCTION(BlueprintCallable, Category=PlayerState)
	FString GetShortPlayerName() const;

	/** Sends kill (excluding self) to clients */
	UFUNCTION(Reliable, Client)
	void InformAboutKill(class AShooterPlayerState* KillerPlayerState, class AShooterPlayerState* KilledPlayerState);

	/** broadcast death to local clients */
	UFUNCTION(Reliable, NetMulticast)
	void BroadcastDeath(class AShooterPlayerState* KillerPlayerState, TSubclassOf<class AShooterWeapon> KillerWeaponClass, TSubclassOf<class UShooterDamageType> KillerDmgType);
	
	UFUNCTION(Reliable, NetMulticast)
	void BroadcastNewColor(uint8 ColorIndex, FLinearColor NewColor, AShooterCharacter* OwnerCharacter);

	/** sets this player's colors, and replicates to the server (and from server to all clients). */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerSetColor(uint8 ColorIndex, FLinearColor NewColor);

	/** sets this player's colors, locally only */
	void SetColorLocal(uint8 ColorIndex, FLinearColor NewColor);

	FLinearColor GetColor(uint8 ColorIndex) const;
	inline int32 GetNumColors() const { return PlayerColors.Num(); } 

	//We don't need stats about amount of ammo fired to be server authenticated, so just increment these with local functions
	void AddBulletsFired(int32 NumBullets);
	void AddRocketsFired(int32 NumRockets);
	
	/** Set whether the player is a quitter */
	UFUNCTION(BlueprintCallable, Category=PlayerState)
	void SetQuitter(bool bInQuitter);

	virtual void CopyProperties(class APlayerState* PlayerState) override;

	inline bool HasLivesRemaining() const { return LivesRemaining != 0; }

	/** reduces LivesRemaining by 1, unless player has infinite lives (-1) */
	void UseLife();

	/** sets this player's lives remaining */
	inline void SetLives(int32 NewLives) { LivesRemaining = NewLives; }

	UFUNCTION(BlueprintCallable, Category=PlayerState)
	int32 GetLivesRemaining() const;

	inline uint8 GetMultiKills() const { return MultiKills; }
	inline uint8 GetKillsSinceLastDeath() const { return KillsSinceLastDeath; }

	/** call this when the player spawns, this will reset their killing spree counter */
	void RestartPlayer();
	
protected:
	
	/** number of lives remaining (-1 = infinite) */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_LivesRemaining)
	int32 LivesRemaining;

	UFUNCTION()
	void OnRep_LivesRemaining();

	/** team number */
	UPROPERTY(Transient, Replicated)
	int32 TeamNumber;
	
	/** color assigned to vector parameter Color1/2/3/etc on all character's meshes */
	UPROPERTY(Transient, Replicated)
	TArray<FLinearColor> PlayerColors;
	
	/** number of kills */
	UPROPERTY(Transient, Replicated)
	int32 NumKills;

	/** number of deaths */
	UPROPERTY(Transient, Replicated)
	int32 NumDeaths;
	
	/** number of kills */
	UPROPERTY(Transient, Replicated)
	int32 NumSuicides;
	
	/** number of bullets fired this match */
	UPROPERTY()
	int32 NumBulletsFired;

	/** number of rockets fired this match */
	UPROPERTY()
	int32 NumRocketsFired;
	
	/** whether the user quit the match */
	UPROPERTY()
	uint8 bQuitter : 1;

	/** helper for scoring points */
	void ScorePoints(int32 Points);

	//////////////////////////////////////////
	// Server only variables

	/** how many kills this player got since their last death, used to determine the killing spree level */
	uint8 KillsSinceLastDeath;

	/** game time that this player last got a kill */
	float LastKillTime;

	/** how many kills this player got in a sequence */
	uint8 MultiKills;
};
