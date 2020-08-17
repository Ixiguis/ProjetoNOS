// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerController.h"
#include "Online.h"
#include "ShooterTypes.h"
#include "Weapons/ShooterWeapon.h"
#include "Weapons/ShooterDamageType.h"
#include "UI/ShooterMessageHandler.h"
#include "Player/ShooterControllerInterface.h"
#include "ShooterPlayerController.generated.h"

UCLASS(config=Game)
class AShooterPlayerController : public APlayerController, public IShooterControllerInterface
{
	GENERATED_BODY()

public:
	AShooterPlayerController();

	virtual void OnRep_PlayerState() override;

	/** [client+server] Called after PlayerState was initialized and replicated */
	UFUNCTION(BlueprintImplementableEvent, Category = Controller)
	void OnPlayerStateReplicated();

	/** sets spectator focus and rotation.
	*	@param bViewTargetIsDead if true, then NewViewTarget is a character and is dead. Must be passed as a parameter from the server 
	*		because AShooterCharacter::Health is not replicated in time for the client to check if NewViewTarget->IsAlive(). */
	UFUNCTION(Reliable, Client)
	void ClientSetSpectatorCamera(AActor* NewViewTarget = NULL, bool bViewTargetIsDead = false);

	/** notify player about started match */
	UFUNCTION(Reliable, Client)
	void ClientGameStarted();

	/** Starts the online game using the session name in the PlayerState */
	UFUNCTION(Reliable, Client)
	void ClientStartOnlineGame();

	FTimerHandle ClientStartOnlineGameHandle;

	/** Ends the online game using the session name in the PlayerState */
	UFUNCTION(Reliable, Client)
	void ClientEndOnlineGame();

	/** notify player about finished match */
	virtual void ClientGameEnded_Implementation(class AActor* EndGameFocus, bool bIsWinner) override;
	
	/** notify player about finished match */
	UFUNCTION(BlueprintImplementableEvent, Category=GameState)
	void NotifyGameEnded();
	
	/** notify about kill */
	UFUNCTION(BlueprintImplementableEvent, Category=GameState, meta=(keywords="killed, score"))
	void NotifyKill(AShooterCharacter* Victim);
	
	/** notify about death */
	UFUNCTION(BlueprintImplementableEvent, Category=GameState, meta=(keywords="hud"))
	void NotifyMessageReceived(const FText& Text, const FColor& TextColor = FColor::White);
	
	/** Notifies clients to send the end-of-round event */
	UFUNCTION(Reliable, Client)
	void ClientSendRoundEndEvent(bool bIsWinner, int32 ExpendedTimeInSeconds);

	/** Sends a FMessage to this player, can be about achievements (Killing Spree, etc), game state (Red flag taken)...
	*	@param MessageType The message type, according to enum EMessageTypes. 
	*	@param bRelativeToMe Whether the message is relative to this player controller. Used to determine first or third person messsages ("You are on a killing spree!" / "Someone's killing spree has ended")
	*	@param OptionalRank Rank, if relevant (e.g. Double Kill = 0, Multi Kill = 1, Tetra Kill = 2, etc.)
	*	@param OptionalTeam If this message is relative to a team, this is the index (e.g. 1 would be blue, "Blue flag taken") */
	UFUNCTION(Reliable, Client)
	void ClientSendMessage(EMessageTypes::Type MessageType, bool bRelativeToMe, const FString& InstigatorName = FString(), const FString& InstigatedName = FString(), uint8 OptionalRank = 0, uint8 OptionalTeam = 0);

	/** used for input simulation from blueprint (for automatic perf tests) */
	UFUNCTION(BlueprintCallable, Category="Input")
	void SimulateInputKey(FKey Key, bool bPressed = true);

	/** sets FOV angle and mouse sensitivity. 
	* @param Zoom Zoom multiplier (0.0 to 1.0)
	* @param MinFOV Minimum FOV allowed (max zoom) */
	UFUNCTION(BlueprintCallable, Category="Camera")
	void SetZoomLevel(float Zoom, float MinFOV);

	float GetDesiredFOV() const;
	/** sends cheat message */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerCheat(const FString& Msg);

	UFUNCTION(Client, Reliable, WithValidation)
	void ClientLoadStreamingLevel(FName LevelPath, int32 UUID);

	/** Local function say a string */
	UFUNCTION(exec)
	virtual void Say(const FString& Msg);

	/** RPC for clients to talk to server */
	UFUNCTION(unreliable, server, WithValidation)
	void ServerSay(const FString& Msg);
	
	/** sets player name and saves it in local profile */
	UFUNCTION(BlueprintCallable, Category=Player)
	void SetPlayerName(const FString& NewName);

	/** Local function run an emote */
// 	UFUNCTION(exec)
// 	virtual void Emote(const FString& Msg);

	/** notify local client about deaths */
	void OnDeathMessage(class AShooterPlayerState* KillerPlayerState, class AShooterPlayerState* KilledPlayerState, TSubclassOf<AShooterWeapon> KillerWeaponClass, TSubclassOf<UShooterDamageType> KillerDmgType);

	/** set infinite ammo cheat */
	UFUNCTION(BlueprintCallable, Category=Cheats)
	void SetInfiniteAmmo(bool bEnable);

	/** set infinite clip cheat */
	UFUNCTION(BlueprintCallable, Category=Cheats)
	void SetInfiniteClip(bool bEnable);

	/** set health regen cheat */
	UFUNCTION(BlueprintCallable, Category=Cheats)
	void SetHealthRegen(bool bEnable);

	/** set god mode cheat */
	UFUNCTION(BlueprintCallable, Category=Cheats)
	void SetGodMode(bool bEnable);

	/** get infinite ammo cheat */
	UFUNCTION(BlueprintCallable, Category=Cheats)
	bool HasInfiniteAmmo() const;

	/** get infinite clip cheat */
	bool HasInfiniteClip() const;

	/** get health regen cheat */
	UFUNCTION(BlueprintCallable, Category=Cheats)
	bool HasHealthRegen() const;

	/** get gode mode cheat */
	UFUNCTION(BlueprintCallable, Category=Cheats)
	bool HasGodMode() const;

	/** check if gameplay related actions (movement, weapon usage, etc) are allowed right now */
	bool IsGameInputAllowed() const;

	/** Sets mouse cursor position to the given screen coordinate. */
	UFUNCTION(BlueprintCallable, Category=Input)
	void SetMousePosition(int32 x, int32 y);
	
	/** Sets mouse cursor position to the screen's center. */
	UFUNCTION(BlueprintCallable, Category=Input)
	void SetMousePositionToCenter();
	
	/** Sets aiming sensitivity with mouse. */
	UFUNCTION(BlueprintCallable, Category=Input)
	void SetMouseSensitivityX(float Sensitivity);
	
	/** Returns current aiming sensitivity with mouse. */
	UFUNCTION(BlueprintCallable, Category=Input)
	float GetMouseSensitivityX() const;

	/** Sets aiming sensitivity with mouse. */
	UFUNCTION(BlueprintCallable, Category = Input)
	void SetMouseSensitivityY(float Sensitivity);

	/** Returns current aiming sensitivity with mouse. */
	UFUNCTION(BlueprintCallable, Category = Input)
	float GetMouseSensitivityY() const;

	/** Ends and/or destroys game session */
	void CleanupSessionOnReturnToMenu();

	/**
	 * Called when the read achievements request from the server is complete
	 *
	 * @param PlayerId The player id who is responsible for this delegate being fired
	 * @param bWasSuccessful true if the server responded successfully to the request
	 */
	void OnQueryAchievementsComplete(const FUniqueNetId& PlayerId, const bool bWasSuccessful );
	
	// Begin APlayerController interface

	/** handle weapon visibility */
	virtual void SetCinematicMode(bool bInCinematicMode, bool bHidePlayer, bool bAffectsHUD, bool bAffectsMovement, bool bAffectsTurning) override;

	/** Returns true if movement input is ignored. Overridden to always allow spectators to move. */
	virtual bool IsMoveInputIgnored() const override;

	/** Returns true if look input is ignored. Overridden to always allow spectators to look around. */
	virtual bool IsLookInputIgnored() const override;

	/** initialize the input system from the player settings */
	virtual void InitInputSystem() override;

	virtual bool SetPause(bool bPause, FCanUnpause CanUnpauseDelegate = FCanUnpause()) override;

	// End APlayerController interface

	// begin AShooterPlayerController-specific

	/** 
	 * Reads achievements to precache them before first use
	 */
	void QueryAchievements();

	/** 
	 * Writes a single achievement (unless another write is in progress).
	 *
	 * @param Id achievement id (string)
	 * @param Percent number 1 to 100
	 */
	void UpdateAchievementProgress( const FString & Id, float Percent );

	/** Returns the persistent user record associated with this player, or null if there isn't one. */
	class UShooterPersistentUser* GetPersistentUser() const;

	/** Informs that player fragged someone */
	void OnKill();
	
	/** Cleans up any resources necessary to return to main menu.  Does not modify GameInstance state. */
	virtual void HandleReturnToMainMenu();

	/** Associate a new UPlayer with this PlayerController. */
	virtual void SetPlayer(UPlayer* Player);

	// end AShooterPlayerController-specific

	virtual void PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel) override;

	/** sends a notify to the HUD, that player picked up a pickup.
	 * @param PickupMessage Message to draw.
	 * @param Pickup Pickup actor, will draw this pickup's icon. */
	UFUNCTION(BlueprintImplementableEvent, Category=Pickup)
	void NotifyPickup(class AShooterPickup* Pickup);

	/** server notifies client that he successfully picked up a pickup.
	 * @param PickupMessage Message to draw.
	 * @param Pickup Pickup actor, will draw this pickup's icon. */
	//UFUNCTION(Client, Reliable, WithValidation)
	//void ClientNotifyPickup(const FText& Text, const int32& Amount, class UTexture2D* LeftImage, class UTexture2D* RightImage);

	// Begin IShooterControllerInterface
	virtual bool IsEnemyFor(AController* TestPC) const;
	// End IShooterControllerInterface

	/** [everyone] called when a player joins the game */
	UFUNCTION(BlueprintImplementableEvent, Category = Pickup)
	void NotifyPlayerJoinedGame(class APlayerState* NewPlayerState);

	/** [everyone] called when a player leaves the game */
	UFUNCTION(BlueprintImplementableEvent, Category = Pickup)
	void NotifyPlayerLeftGame(class APlayerState* LeavingPlayerState);

	virtual void ToggleSpeaking(bool bSpeaking);

	/** Reads the UserSettings for chat option, and registers or unregisters the voice interface. */
	void UpdateChatOption();

	virtual void ReceivedGameModeClass(TSubclassOf<class AGameModeBase> GameModeClass) override;

	/** [server+client] Called after the gamemode and gamestate are initialized. */
	UFUNCTION(BlueprintImplementableEvent, Category = Controller)
	void GameModeAndStateInitialized();

	virtual void AcknowledgePossession(class APawn* P) override;

	/** [server+client] Called when the controller possesses a new pawn. Unlike AController::ReceivePossess, this is called on clients too. */
	UFUNCTION(BlueprintImplementableEvent, Category = Controller)
	void Possessed(class APawn* NewPawn);

	/** Called when the weapon has run SetCrosshair() */
	UFUNCTION(BlueprintImplementableEvent, Category = "Controller")
	void CrosshairChanged(UTexture2D* NewCrosshair);

	/** Sends a physic body's data to the client, to keep it in sync with the server. */
	UFUNCTION(Client, Unreliable)
	void ClientUpdatePhysicsBody(UPrimitiveComponent* PhysBody, const FVector_NetQuantize& Loc, const FVector_NetQuantize& LinearVel, const FVector_NetQuantize& AngularVel, const FQuat& Rot);

	/** Wrapper to call the above function ("Sends a physic body's data to the client, to keep it in sync with the server.") */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Physics")
	void ClientUpdatePhysicsBody_(UPrimitiveComponent* PhysBody);

protected:

	/** infinite ammo cheat */
	UPROPERTY(Transient, Replicated)
	uint8 bInfiniteAmmo : 1;

	/** infinite clip cheat */
	UPROPERTY(Transient, Replicated)
	uint8 bInfiniteClip : 1;

	/** health regen cheat */
	UPROPERTY(Transient, Replicated)
	uint8 bHealthRegen : 1;

	/** god mode cheat */
	UPROPERTY(Transient, Replicated)
	uint8 bGodMode : 1;

	/** if set, gameplay related actions (movement, weapn usage, etc) are allowed */
	uint8 bAllowGameActions : 1;
	
	/** true for the first frame after the game has ended */
	uint8 bGameEndedFrame : 1;

	/** stores pawn location at last player death, used where player scores a kill after they died **/
	FVector LastDeathLocation;

	/** default (initial) mouse sensitivity */
	float DefaultMouseSensitivity;

	/** FOV + mouse sensitivity  adjustment */
	float ZoomLevel;

	/** FOV to be set by camera update */
	float DesiredFOV;
	
	/** Saved on game start */
	float DefaultFOV;

	/** Achievements write object */
	FOnlineAchievementsWritePtr WriteObject;

	//Begin AActor interface

	/** after all game elements are created */
	virtual void PostInitializeComponents() override;

	virtual void BeginPlay() override;

	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	//End AActor interface

	//Begin AController interface
	virtual void InitPlayerState() override;

	/** transition to dead state, retries spawning later */
	virtual void FailedToSpawnPawn() override;
	
	/** update camera when pawn dies */
	virtual void PawnPendingDestroy(APawn* P) override;

	//End AController interface

	// Begin APlayerController interface

	/** respawn after dying */
	virtual void UnFreeze() override;

	/** sets up input */
	virtual void SetupInputComponent() override;

	/**
	 * Called from game info upon end of the game, used to transition to proper state. 
	 *
	 * @param EndGameFocus Actor to set as the view target on end game
	 * @param bIsWinner true if this controller is on winning team
	 */
	virtual void GameHasEnded(class AActor* EndGameFocus = NULL, bool bIsWinner = false) override;	

	/** Return the client to the main menu gracefully.  ONLY sets GI state. */
	void ClientReturnToMainMenu_Implementation(const FString& ReturnReason) override;

	/** Causes the player to commit suicide */
	UFUNCTION(exec)
	virtual void Suicide();
	
	UFUNCTION(exec)
	virtual void SetSoundClassVolume(FString ClassName, float NewVolume);

	/** Notifies the server that the client has suicided */
	UFUNCTION(Reliable, server, WithValidation)
	void ServerSuicide();

	/** Updates achievements based on the PersistentUser stats at the end of a round */
	void UpdateAchievementsOnGameEnd();
	// End APlayerController interface

	FName	ServerSayString;
	
	// Timer used for updating friends in the player tick.
	float ShooterFriendUpdateTimer;

	// For tracking whether or not to send the end event
	bool bHasSentStartEvents;

	FOnPlayerTalkingStateChangedDelegate TalkingStateChangedDelegate;

	void TalkingStateChanged(TSharedRef<const FUniqueNetId> TalkerId, bool bIsTalking);

	int32 NumberOfTalkers;

	void AddLocalPlayer();
	void RemoveLocalPlayer();
};
