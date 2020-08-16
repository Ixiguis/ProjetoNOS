// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.
#pragma once

#include "GameFramework/GameMode.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "ShooterTypes.h"
#include "UI/ShooterMessageHandler.h"
#include "ShooterGameMode.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogShooterGameMode, Log, All);

UCLASS(config=Game)
class AShooterGameMode : public AGameMode
{
	GENERATED_UCLASS_BODY()

public:

	//UObject interface
	virtual void PostInitProperties() override;
	// End of UObject interface

	/** The bot pawn class */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=GameMode)
	TSubclassOf<class APawn> BotPawnClass;

	UFUNCTION(exec)
	void SetAllowBots(bool bInAllowBots, int32 InMaxBots = 63);

	virtual void InitGameState() override;

	/** Initialize the game. This is called before actors' PreInitializeComponents. */
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/** Accept or reject a player attempting to join the server.  Fails login if you set the ErrorMessage to a non-empty string. */
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	/** starts match warmup */
	virtual void PostLogin(APlayerController* NewPlayer) override;
	
	/** check if player has lives remaining */
	virtual void RestartPlayer(AController* NewPlayer) override;

	/** Called when a Controller with a PlayerState leaves the match. */
	virtual void Logout( AController* Exiting );

	/** disable respawns outside current match */
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;

	/** select best spawn point for player */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	/** always pick new random spawn */
	virtual bool ShouldSpawnAtStartSpot(AController* Player) override;

	/** returns default pawn class for given controller */
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	/** modify damage based on game rules */
	UFUNCTION(BlueprintCallable, Category=GameMode)
	virtual float ModifyDamage(float Damage, AActor* DamagedActor, AController* EventInstigator) const;

	/** notify about kills */
	virtual void Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, TSubclassOf<class AShooterWeapon> KillerWeaponClass, TSubclassOf<class UShooterDamageType> KillerDmgType);

	/** can players damage each other? */
	virtual bool CanDealDamage(class AShooterPlayerState* DamageInstigator, class AShooterPlayerState* DamagedPlayer) const;

	/** always create cheat manager */
	virtual bool AllowCheats(APlayerController* P) override;
	
	/** called before startmatch */
	virtual void HandleMatchIsWaitingToStart() override;

	/** starts new match */
	virtual void HandleMatchHasStarted() override;
	
	/** hides the onscreen hud and restarts the map */
	virtual void RestartGame() override;

	/** Creates AIControllers for all bots */
	void CreateBotControllers();

	/** Create a bot */
	class AShooterAIController* CreateBot(int32 BotNum);	

	/** Defines all bots colors */
	void AssignBotsColors();

	/** respawn timer multiplier for all pickups*/
	UPROPERTY(config)
	float GlobalPickupRespawnTimeMultipler;
	
	/** weapons stay when picked up (but can only be picked up if character doesn't have it already) */
	UPROPERTY(config)
	bool WeaponStay;
	
	/** characters should drop weapon+ammo on death? */
	UPROPERTY(config)
	bool DropWeaponOnDeath;
	
	/** characters should drop powerup (if any) on death? */
	UPROPERTY(config)
	bool DropPowerupOnDeath;

	/** how long dropped items stay on ground */
	UPROPERTY(config)
	float DroppedPickupDuration;
	
	/** Whether this game mode should notify clients about game achievements (double kill, killing spree, etc.) */
	UPROPERTY(config)
	bool NotifyGameAchievements;
	
	/** True: Client will verify for instant-hit weapons hits and send any hits to server for verification. ("zero-ping" play; may be hacked)
	*	False: Server will process all instant-hit weapons and notify clients in case of hits. (non "zero-ping" play, cannot be hacked) */
	UPROPERTY(config)
	bool bClientSideHitVerification;

	/** True: each projectile fired will be replicated from server to clients. This uses more bandwidth but is very accurate.
	*	False (recommended): each client will create projectiles on their machines based on whether other players are shooting or not. This uses less bandwidth but is slightly less accurate. */
	UPROPERTY(config)
	bool bReplicateProjectiles;

	/** Additional information about this game mode */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=GameMode)
	FGameModeInfo GameModeInfo;
	
	/** Returns this game mode's short name (e.g. "DM", "CTF", etc.) */
	UFUNCTION(BlueprintCallable, Category=GameMode)
	FString GetGameModeShortName() const;
	
	inline TArray<struct FGameModeInfo> GetGameModeList() { return GameModeList; }

protected:
	
	/////////////////////////////////////////////////
	// GAME SETTINGS

	/** if true, then match won't end due to time */
	bool bUnlimitedRoundTime;

	/** game mode aliases (CTF, DM, etc.) */
	UPROPERTY(config)
	TArray<struct FGameModeInfo> GameModeList;

	/** delay between first player login and starting match */
	UPROPERTY(config)
	int32 WarmupTime;
	
	/** number of teams */
	UPROPERTY(BlueprintReadWrite, Category=Options, config)
	uint8 NumTeams;
	
	/** score limit -- first to it wins */
	UPROPERTY(BlueprintReadWrite, Category=Options, config, meta=(keywords="timelimit"))
	int32 ScoreLimit;

	/** match duration */
	UPROPERTY(BlueprintReadWrite, Category=Options, config)
	int32 RoundTime;

	UPROPERTY(config)
	int32 TimeBetweenMatches;

	/** score for kill */
	UPROPERTY(config)
	int32 KillScore;

	/** score for death */
	UPROPERTY(config)
	int32 DeathScore;
	
	/** score for suicide */
	UPROPERTY(config)
	int32 SuicideScore;
	
	/** scale for self instigated damage */
	UPROPERTY(config)
	float DamageSelfScale;
	
	UPROPERTY(config)
	int32 MaxBots;
	
	UPROPERTY()
	TArray<class AShooterAIController*> BotControllers;
	
	class AShooterGameState* ShooterGameState;

	bool bNeedsBotCreation;

	void StartBots();

	/** initialization for bot after creation */
	virtual void InitBot(class AShooterAIController* AIC, int32 BotNum);

	/** check who won. Returns the focus actor. */
	virtual class AActor* DetermineMatchWinner();

	/** check if PlayerState is a winner */
	virtual bool IsWinner(class AShooterPlayerState* PlayerState) const;

	/** check if player can use spawnpoint */
	virtual bool IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const;

	/** check if any other player overlaps this spawn point */
	virtual bool AnyPawnOverlapsSpawnPoint(APlayerStart* SpawnPoint, AController* Player) const;
	
	/** returns the best spawn point from the list, depending on game rules */
	virtual APlayerStart* GetBestSpawnPoint(const TArray<APlayerStart*>& AvailableSpawns, AController* Player) const;

	/** Returns game session class to use */
	virtual TSubclassOf<AGameSession> GetGameSessionClass() const override;

	/** Returns true if any player has lives remaining. */
	bool AnyPlayerHasLivesRemaining() const;

	/** Restarts all players, if they have enough lives. 
	*	@param bDeadOnly	If true, only restart dead players. */
	virtual void RestartAllPlayers(bool bDeadOnly = true);
	
	virtual void DefaultTimer() {}

	FTimerHandle DefaultTimerHandle;
	
	FTimerHandle SyncPhysBodiesHandle;

	/** interval (seconds) between physics bodies synchronization. Set to zero to disable this. */
	UPROPERTY(config)
	float PhysicsBodiesSyncTime;
	
	/** Whether to sync ragdolls (corpses) locations between server and clients */
	UPROPERTY(config)
	bool bSyncRagdolls;

public:

	/** finish current match and lock players */
	UFUNCTION(exec)
	virtual void FinishMatch();
	
	/*Finishes the match and bumps everyone to main menu.*/
	/*Only GameInstance should call this function */
	void RequestFinishAndExitToMainMenu();

	/** get the name of the bots count option used in server travel URL */
	static FString GetBotsCountOptionName();

	int32 GetWarmupTime();

	/** User settings pointer */
	class UShooterGameUserSettings* UserSettings;

	/** sets all player's remaining lives to NewRemainingLives */
	void SetPlayersLifes(int32 NewRemainingLifes);
	
	/** Checks if the player scored an achievement (double kill, killing spree, etc) on his latest kill. Called by the killed ShooterCharacter::Die(). */
	virtual void CheckAndNotifyAchievements(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, class AActor* DamageCauser);

	/** Calls PC->ClientSendMessage for all player controllers. */
	void MessagePlayers(TEnumAsByte<EMessageTypes::Type> MessageType, AController* MessageRelativeTo, const FString& InstigatorName = FString(), const FString& InstigatedName = FString(), uint8 OptionalRank = 0, uint8 OptionalTeam = 0) const;
	void MessagePlayers(TEnumAsByte<EMessageTypes::Type> MessageType, AController* MessageRelativeTo, const APawn* Instigator = NULL, const APawn* Instigated = NULL, uint8 OptionalRank = 0, uint8 OptionalTeam = 0) const;

	private:

	/** sends the location, velocity, and rotation of all physics bodies from server to clients, to ensure they are in the same place */
	void SyncPhysicsBodies();

	/** every 10 iterations, a full sync will be performed (regardless whether the rigid bodies are asleep or not) */
	int32 PhysicsBodiesSyncCounter;

};

FORCEINLINE int32 AShooterGameMode::GetWarmupTime()
{
	return WarmupTime;
}