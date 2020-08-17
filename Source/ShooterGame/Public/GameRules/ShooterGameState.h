// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once

#include "GameFramework/GameState.h"
#include "UI/ShooterMessageHandler.h"
#include "ShooterGameState.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogShooterGameState, Log, All);

UCLASS(config = Game)
class AShooterGameState : public AGameState
{
	GENERATED_BODY()

public:
	AShooterGameState();

	/** time left for warmup / match */
	UPROPERTY(BlueprintReadOnly, Category=GameState, Transient, Replicated)
	int32 RemainingTime;

	/** is timer paused? */
	UPROPERTY(BlueprintReadOnly, Category=GameState, Transient, Replicated)
	bool bTimerPaused;

	/////////////////////////
	// Team game variables
	
	/** Whether players should have their colors changed, according to their team number. */
	UPROPERTY(BlueprintReadOnly, Category=GameState, Transient, Replicated)
	uint32 bChangeToTeamColors : 1;
	
	/** if true, each player in their team adds/removes points for their team. If false, individual player's scores aren't counted to their team score. */
	UPROPERTY(BlueprintReadOnly, Category=GameState, Transient, Replicated)
	uint32 bPlayersAddTeamScore : 1;
	
	/////////////////////////
	// Weapon usage

	/** True: Client will verify for instant-hit weapons hits and send any hits to server for verification. ("zero-ping" play; may be hacked)
	*	False: Server will process all instant-hit weapons and only notify clients in case of hits. (non "zero-ping" play, cannot be hacked) */
	UPROPERTY(Config, BlueprintReadOnly, Category=GameState, Transient, Replicated)
	bool bClientSideHitVerification;

	/** True: each projectile will be replicated from server to clients. This uses more bandwidth but is very accurate.
	*	False: each client will create projectiles on their machines based on whether other clients are shooting or not. This uses much less bandwidth but is less accurate. */
	UPROPERTY(Config, BlueprintReadOnly, Category=GameState, Transient, Replicated)
	bool bReplicateProjectiles;
	
protected:
	/** accumulated score per team */
	UPROPERTY(BlueprintReadOnly, Category=GameState, Transient, Replicated)
	TArray<int32> TeamScores;
	
	/** number of teams in current game */
	UPROPERTY(BlueprintReadOnly, Category=GameState, Transient, Replicated)
	uint8 NumTeams;

	/** allocates TeamScores, according to the number of teams */
	void InitTeamScores();

	// Begin AActor interface
	virtual void PostInitializeComponents() override;
	// End AActor interface
	
	/** contains data about game messages, like "double kill", "killing spree", etc. */
	UPROPERTY(Transient)
	class UShooterMessageHandler* MessageHandlerInst;

	/** message handler blueprint class reference */
	UPROPERTY(EditDefaultsOnly, Category = GameState)
	TSubclassOf<class UShooterMessageHandler> MessageHandlerClass;

public:

	UFUNCTION(BlueprintPure, Category = GameState)
	int32 GetTeamScore(uint8 TeamNum) const;

	UFUNCTION(BlueprintPure, Category = GameState)
	uint8 GetNumTeams() const;

	UFUNCTION(BlueprintCallable, Category=GameState)
	void SetNumTeams(uint8 n);
	
	UFUNCTION(BlueprintCallable, Category=GameState)
	void AddTeamScore(uint8 TeamNumber, int32 ScoreToAdd);
	
	UFUNCTION(BlueprintCallable, Category=GameState)
	void SetTeamScore(uint8 TeamNumber, int32 NewScoe);

	/** returns the total number of kills that every player got this match (doesn't counts suicides) */
	UFUNCTION(BlueprintCallable, Category=GameState)
	int32 GetTotalKills();

	/** returns whether this is a team game */
	UFUNCTION(BlueprintPure, Category = GameState)
	bool IsTeamGame() const;

	/** returns Player's position in the match, [1..n] */
	UFUNCTION(BlueprintPure, Category = GameState)
	int32 GetPlayerPosition(class AShooterPlayerState* Player) const;

	/** returns Player's team position in the match, [1..n] */
	UFUNCTION(BlueprintPure, Category = GameState)
	int32 GetPlayersTeamPosition(class AShooterPlayerState* Player) const;

	/** returns the number of players. If Team == -1, returns the total number of players; otherwise, returns given team's number of players. */
	UFUNCTION(BlueprintPure, Category = GameState)
	int32 GetNumberOfPlayers(int32 Team = -1) const;

	class UShooterMessageHandler* GetMessageHandler() const;
	
	/** returns a FGameMessage from given MessageType */
	FGameMessage GetGameMessage(TEnumAsByte<EMessageTypes::Type> MessageType, uint8 OptionalRank = 0) const;
	
	/////////////////////////
	// Misc variables

	/** gets ranked PlayerState map for specific team (first rank = position 0) */
	UFUNCTION(BlueprintPure, Category = GameState)
	TArray<class AShooterPlayerState*> GetRankedPlayerArray(int32 TeamIndex) const;
	
	void RequestFinishAndExitToMainMenu();
};
