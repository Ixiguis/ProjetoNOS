// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "Player/ShooterPlayerController.h"
#include "Online.h"
#include "Interfaces/OnlineEventsInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "ShooterGameUserSettings.h"
#include "ShooterGameViewportClient.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerInput.h"
#include "Player/ShooterCharacter.h"
#include "Player/ShooterPlayerState.h"
#include "Player/ShooterLocalPlayer.h"
#include "Player/ShooterSpectatorPawn.h"
#include "Player/ShooterPlayerCameraManager.h"
#include "Player/ShooterCheatManager.h"
#include "Player/ShooterPersistentUser.h"
#include "GameRules/ShooterGameMode.h"
#include "ShooterLeaderboards.h"
#include "Net/UnrealNetwork.h"

#define  ACH_FRAG_SOMEONE	TEXT("ACH_FRAG_SOMEONE")
#define  ACH_SOME_KILLS		TEXT("ACH_SOME_KILLS")
#define  ACH_LOTS_KILLS		TEXT("ACH_LOTS_KILLS")
#define  ACH_FINISH_MATCH	TEXT("ACH_FINISH_MATCH")
#define  ACH_LOTS_MATCHES	TEXT("ACH_LOTS_MATCHES")
#define  ACH_FIRST_WIN		TEXT("ACH_FIRST_WIN")
#define  ACH_LOTS_WIN		TEXT("ACH_LOTS_WIN")
#define  ACH_MANY_WIN		TEXT("ACH_MANY_WIN")
#define  ACH_SHOOT_BULLETS	TEXT("ACH_SHOOT_BULLETS")
#define  ACH_SHOOT_ROCKETS	TEXT("ACH_SHOOT_ROCKETS")
#define  ACH_GOOD_SCORE		TEXT("ACH_GOOD_SCORE")
#define  ACH_GREAT_SCORE	TEXT("ACH_GREAT_SCORE")
#define  ACH_PLAY_SANCTUARY	TEXT("ACH_PLAY_SANCTUARY")
#define  ACH_PLAY_HIGHRISE	TEXT("ACH_PLAY_HIGHRISE")

static const int32 SomeKillsCount = 10;
static const int32 LotsKillsCount = 20;
static const int32 LotsMatchesCount = 5;
static const int32 LotsWinsCount = 3;
static const int32 ManyWinsCount = 5;
static const int32 LotsBulletsCount = 100;
static const int32 LotsRocketsCount = 10;
static const int32 GoodScoreCount = 10;
static const int32 GreatScoreCount = 15;

AShooterPlayerController::AShooterPlayerController()
{
	bShowMouseCursor = false;
	PlayerCameraManagerClass = AShooterPlayerCameraManager::StaticClass();
	CheatClass = UShooterCheatManager::StaticClass();
	bAllowGameActions = true;
	LastDeathLocation = FVector::ZeroVector;

	ZoomLevel = 1.0;
	ServerSayString = TEXT("Say");
	ShooterFriendUpdateTimer = 0.0f;
	bHasSentStartEvents = false;
	NumberOfTalkers = 0;
	bCheckForGameAndPlayerState = true;
}

void AShooterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// chat
	InputComponent->BindAction("PushToTalk", IE_Pressed, this, &APlayerController::StartTalking);
	InputComponent->BindAction("PushToTalk", IE_Released, this, &APlayerController::StopTalking);

	InputComponent->BindAction("AddLocalPlayer", IE_Pressed, this, &AShooterPlayerController::AddLocalPlayer);
	InputComponent->BindAction("RemoveLocalPlayer", IE_Pressed, this, &AShooterPlayerController::RemoveLocalPlayer);
}


void AShooterPlayerController::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void AShooterPlayerController::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);
	
	// Is this the first frame after the game has ended
	if(bGameEndedFrame)
	{
		bGameEndedFrame = false;
	}
	if (bCheckForGameAndPlayerState)
	{
		if (GetPlayerState<APlayerState>() && GetWorld()->GetGameState())
		{
			OnGameAndPlayerStateReplicated();
			bCheckForGameAndPlayerState = false;
		}
	}
}

void AShooterPlayerController::SetPlayer(UPlayer* InPlayer)
{
	Super::SetPlayer(InPlayer);
	DesiredFOV = DefaultFOV = PlayerCameraManager->GetFOVAngle();
	UpdateChatOption();

}

void AShooterPlayerController::QueryAchievements()
{
	// precache achievements
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer)
	{
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if(OnlineSub)
		{
			IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
			if (Identity.IsValid())
			{
				TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(LocalPlayer->GetControllerId());

				if (UserId.IsValid())
				{
					IOnlineAchievementsPtr Achievements = OnlineSub->GetAchievementsInterface();

					if (Achievements.IsValid())
					{
						Achievements->QueryAchievements( *UserId.Get(), FOnQueryAchievementsCompleteDelegate::CreateUObject( this, &AShooterPlayerController::OnQueryAchievementsComplete ));
					}
				}
				else
				{
					UE_LOG(LogOnline, Warning, TEXT("No valid user id for this controller."));
				}
			}
			else
			{
				UE_LOG(LogOnline, Warning, TEXT("No valid identity interface."));
			}
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("No default online subsystem."));
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("No local player, cannot read achievements."));
	}
}

void AShooterPlayerController::OnQueryAchievementsComplete(const FUniqueNetId& PlayerId, const bool bWasSuccessful )
{
	UE_LOG(LogOnline, Display, TEXT("AShooterPlayerController::OnQueryAchievementsComplete(bWasSuccessful = %s)"), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));
}

void AShooterPlayerController::UnFreeze()
{
	bPlayerIsWaiting = true;
	ServerRestartPlayer();
}

void AShooterPlayerController::FailedToSpawnPawn()
{
	if(StateName == NAME_Inactive)
	{
		BeginInactiveState();
	}
	Super::FailedToSpawnPawn();
}

void AShooterPlayerController::PawnPendingDestroy(APawn* P)
{
	APawn* DestroyedPawn = P;
	LastDeathLocation = P->GetActorLocation();

	Super::PawnPendingDestroy(P);

	ClientSetSpectatorCamera(DestroyedPawn, true);
}

void AShooterPlayerController::OnPossess(APawn* aPawn)
{
	Super::OnPossess(aPawn);
	//called on server
	OnPossessed(aPawn);
}

void AShooterPlayerController::GameHasEnded(class AActor* EndGameFocus, bool bIsWinner)
{
	// write stats
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer)
	{
		AShooterPlayerState* ShooterPlayerState = GetPlayerState<AShooterPlayerState>();
		if (ShooterPlayerState)
		{
			// update local saved profile
			UShooterPersistentUser* const PersistentUser = GetPersistentUser();
			if (PersistentUser)
			{
				PersistentUser->AddMatchResult( ShooterPlayerState->GetKills(), ShooterPlayerState->GetDeaths(), ShooterPlayerState->GetSuicides(), bIsWinner);
				PersistentUser->SaveIfDirty();
			}

			// update achievements
			UpdateAchievementsOnGameEnd();
			
			// update leaderboards
			IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
			if(OnlineSub)
			{
				IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
				if (Identity.IsValid())
				{
					TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(LocalPlayer->GetControllerId());
					if (UserId.IsValid())
					{
						IOnlineLeaderboardsPtr Leaderboards = OnlineSub->GetLeaderboardsInterface();
						if (Leaderboards.IsValid())
						{
							FShooterAllTimeMatchResultsWrite WriteMatchResultsObject;

							WriteMatchResultsObject.SetIntStat(LEADERBOARD_STAT_SCORE, ShooterPlayerState->GetKills());
							WriteMatchResultsObject.SetIntStat(LEADERBOARD_STAT_KILLS, ShooterPlayerState->GetKills());
							WriteMatchResultsObject.SetIntStat(LEADERBOARD_STAT_DEATHS, ShooterPlayerState->GetDeaths());
//							WriteMatchResultsObject.SetIntStat(LEADERBOARD_STAT_SUICIDES, ShooterPlayerState->GetSuicides());
							WriteMatchResultsObject.SetIntStat(LEADERBOARD_STAT_MATCHESPLAYED, 1);
			
							// the call will copy the user id and write object to its own memory
//							Leaderboards->WriteLeaderboards(ShooterPlayerState->SessionName, *UserId, WriteObject);
						}						
					}
				}
			}
		}
	}

	Super::GameHasEnded(EndGameFocus, bIsWinner);
}

void AShooterPlayerController::ClientSetSpectatorCamera_Implementation(AActor* NewViewTarget, bool bViewTargetIsDead)
{
	ChangeState(NAME_Spectating);
	AShooterSpectatorPawn* Spec = Cast<AShooterSpectatorPawn>(GetSpectatorPawn());
	if (Spec)
	{
		Spec->SetViewTarget(NewViewTarget, bViewTargetIsDead);
		AShooterPlayerState* PS = GetPlayerState<AShooterPlayerState>();
		if (PS && !PS->HasLivesRemaining())
		{
			Spec->bAllowFreeCam = true;
			Spec->bAllowSwitchFocus = true;
		}
	}
}

bool AShooterPlayerController::ServerCheat_Validate(const FString& Msg)
{
	return true;
}

void AShooterPlayerController::ServerCheat_Implementation(const FString& Msg)
{
	if (CheatManager)
	{
		ClientMessage(ConsoleCommand(Msg));
	}
}

void AShooterPlayerController::SimulateInputKey(FKey Key, bool bPressed)
{
	InputKey(Key, bPressed ? IE_Pressed : IE_Released, 1, false);
}

void AShooterPlayerController::OnKill()
{
	UpdateAchievementProgress(ACH_FRAG_SOMEONE, 100.0f);

	const auto Events = Online::GetEventsInterface();
	const auto Identity = Online::GetIdentityInterface();

	if (Events.IsValid() && Identity.IsValid())
	{
		ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
		if (LocalPlayer)
		{
			int32 UserIndex = LocalPlayer->GetControllerId();
			TSharedPtr<const FUniqueNetId> UniqueID = Identity->GetUniquePlayerId(UserIndex);			
			if (UniqueID.IsValid())
			{
				AShooterCharacter* MyPawn = Cast<AShooterCharacter>(GetCharacter());
				// If player is dead, use location stored during pawn cleanup.
				FVector Location = MyPawn ? MyPawn->GetActorLocation() : LastDeathLocation;

				AShooterWeapon* Weapon = MyPawn ? MyPawn->GetWeapon() : 0;
				int32 WeaponType = 0;//Weapon ? (int32)Weapon->GetAmmoType() : 0;

				FOnlineEventParms Params;		

				Params.Add( TEXT( "SectionId" ), FVariantData( (int32)0 ) ); // unused
				Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
				Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused

				Params.Add( TEXT( "PlayerRoleId" ), FVariantData( (int32)0 ) ); // unused
				Params.Add( TEXT( "PlayerWeaponId" ), FVariantData( (int32)WeaponType ) );
				Params.Add( TEXT( "EnemyRoleId" ), FVariantData( (int32)0 ) ); // unused
				Params.Add( TEXT( "EnemyWeaponId" ), FVariantData( (int32)0 ) ); // untracked			
				Params.Add( TEXT( "KillTypeId" ), FVariantData( (int32)0 ) ); // unused	
				Params.Add( TEXT( "LocationX" ), FVariantData( Location.X ) );
				Params.Add( TEXT( "LocationY" ), FVariantData( Location.Y ) );
				Params.Add( TEXT( "LocationZ" ), FVariantData( Location.Z ) );	
			
				Events->TriggerEvent(*UniqueID, TEXT("KillOponent"), Params);				
			}
		}
	}
}

void AShooterPlayerController::OnDeathMessage(class AShooterPlayerState* KillerPlayerState, class AShooterPlayerState* KilledPlayerState, TSubclassOf<class AShooterWeapon> KillerWeaponClass, TSubclassOf<class UShooterDamageType> KillerDmgType)
{
	/*AShooterHUD* ShooterHUD = GetShooterHUD();
	if (ShooterHUD)
	{
		ShooterHUD->ShowDeathMessage(KillerPlayerState, KilledPlayerState, KillerWeaponClass, KillerDmgType);
	}

	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer && LocalPlayer->GetCachedUniqueNetId().IsValid() && KilledPlayerState->UniqueId.IsValid())
	{
		// if this controller is the player who died, update the hero stat.
		if (*LocalPlayer->GetCachedUniqueNetId() == *KilledPlayerState->UniqueId)
		{
			const auto Events = Online::GetEventsInterface();
			const auto Identity = Online::GetIdentityInterface();

			if (Events.IsValid() && Identity.IsValid())
			{							
				int32 UserIndex = LocalPlayer->GetControllerId();
				TSharedPtr<const FUniqueNetId> UniqueID = Identity->GetUniquePlayerId(UserIndex);
				if (UniqueID.IsValid())
				{				
					AShooterCharacter* Pawn = Cast<AShooterCharacter>(GetCharacter());
					AShooterWeapon* Weapon = Pawn ? Pawn->GetWeapon() : NULL;
					FVector Location = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
					int32 WeaponType = 0;//Weapon ? (int32)Weapon->GetAmmoType() : 0;

					FOnlineEventParms Params;
					Params.Add( TEXT( "SectionId" ), FVariantData( (int32)0 ) ); // unused
					Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
					Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused

					Params.Add( TEXT( "PlayerRoleId" ), FVariantData( (int32)0 ) ); // unused
					Params.Add( TEXT( "PlayerWeaponId" ), FVariantData( (int32)WeaponType ) );
					Params.Add( TEXT( "EnemyRoleId" ), FVariantData( (int32)0 ) ); // unused
					Params.Add( TEXT( "EnemyWeaponId" ), FVariantData( (int32)0 ) ); // untracked
				
					Params.Add( TEXT( "LocationX" ), FVariantData( Location.X ) );
					Params.Add( TEXT( "LocationY" ), FVariantData( Location.Y ) );
					Params.Add( TEXT( "LocationZ" ), FVariantData( Location.Z ) );
										
					Events->TriggerEvent(*UniqueID, TEXT("PlayerDeath"), Params);
				}
			}
		}
	}	*/
}

void AShooterPlayerController::UpdateAchievementProgress( const FString& Id, float Percent )
{
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer)
	{
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if(OnlineSub)
		{
			IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
			if (Identity.IsValid())
			{
				FUniqueNetIdRepl UserId = LocalPlayer->GetCachedUniqueNetId();

				if (UserId.IsValid())
				{

					IOnlineAchievementsPtr Achievements = OnlineSub->GetAchievementsInterface();
					if (Achievements.IsValid() && (!WriteObject.IsValid() || WriteObject->WriteState != EOnlineAsyncTaskState::InProgress))
					{
						WriteObject = MakeShareable(new FOnlineAchievementsWrite());
						WriteObject->SetFloatStat(*Id, Percent);
						FOnlineAchievementsWriteRef WriteObjectRef = WriteObject.ToSharedRef();
						Achievements->WriteAchievements(*UserId, WriteObjectRef);
					}
					else
					{
						UE_LOG(LogOnline, Warning, TEXT("No valid achievement interface or another write is in progress."));
					}
				}
				else
				{
					UE_LOG(LogOnline, Warning, TEXT("No valid user id for this controller."));
				}
			}
			else
			{
				UE_LOG(LogOnline, Warning, TEXT("No valid identity interface."));
			}
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("No default online subsystem."));
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("No local player, cannot update achievements."));
	}
}

void AShooterPlayerController::SetInfiniteAmmo(bool bEnable)
{
	bInfiniteAmmo = bEnable;
}

void AShooterPlayerController::SetInfiniteClip(bool bEnable)
{
	bInfiniteClip = bEnable;
}

void AShooterPlayerController::SetHealthRegen(bool bEnable)
{
	bHealthRegen = bEnable;
}

void AShooterPlayerController::SetGodMode(bool bEnable)
{
	bGodMode = bEnable;
}

void AShooterPlayerController::ClientGameStarted_Implementation()
{
	bAllowGameActions = true;

	// Enable controls and disable cinematic mode now the game has started
	SetCinematicMode(false, false, true, true, true);

	//@HUD: set match state (playing), hide scoreboard

	QueryAchievements();
	

	// Send round start event
	const auto Events = Online::GetEventsInterface();
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);

	if(LocalPlayer != nullptr && Events.IsValid())
	{
		auto UniqueId = LocalPlayer->GetPreferredUniqueNetId();

		if (UniqueId.IsValid())
		{
			// Generate a new session id
			Events->SetPlayerSessionId(*UniqueId, FGuid::NewGuid());

			FString MapName = *FPackageName::GetShortName(GetWorld()->PersistentLevel->GetOutermost()->GetName());

			// Fire session start event for all cases
			FOnlineEventParms Params;
			Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
			Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused
			Params.Add( TEXT( "MapName" ), FVariantData( MapName ) );
			
			Events->TriggerEvent(*UniqueId, TEXT("PlayerSessionStart"), Params);


			bHasSentStartEvents = true;
		}
	}

	//send player colors and name to the server
	UShooterPersistentUser* MySPU = GetPersistentUser();
	AShooterPlayerState* PS = GetPlayerState<AShooterPlayerState>();
	if (MySPU && PS)
	{
		for (int32 i=0; i<MySPU->GetNumColors(); i++)
		{
			PS->ServerSetColor(i, MySPU->GetColor(i));
		}
		//set player's name
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub && MySPU->PlayerNameIsDefault())
		{
			const FString PlayerName = OnlineSub->GetIdentityInterface()->GetPlayerNickname(0);
			if (PlayerName.Len() > 0)
			{
				SetName(PlayerName);
				MySPU->SetPlayerName(PlayerName);
			}
		}
		else
		{
			SetName(MySPU->GetPlayerName());
		}
	}
}

/** Starts the online game using the session name in the PlayerState */
void AShooterPlayerController::ClientStartOnlineGame_Implementation()
{
	if (!IsPrimaryPlayer())
		return;

	AShooterPlayerState* ShooterPlayerState = GetPlayerState<AShooterPlayerState>();
	if (ShooterPlayerState)
	{
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid())
			{
				UE_LOG(LogOnline, Log, TEXT("Starting session %s on client"), *ShooterPlayerState->SessionName.ToString() );
				Sessions->StartSession(ShooterPlayerState->SessionName);
			}
		}
	}
	else
	{
		// Keep retrying until player state is replicated
		GetWorld()->GetTimerManager().SetTimer(ClientStartOnlineGameHandle, this, &AShooterPlayerController::ClientStartOnlineGame_Implementation, 0.2f, false);
	}
}

/** Ends the online game using the session name in the PlayerState */
void AShooterPlayerController::ClientEndOnlineGame_Implementation()
{
	if (!IsPrimaryPlayer())
		return;

	AShooterPlayerState* ShooterPlayerState = GetPlayerState<AShooterPlayerState>();
	if (ShooterPlayerState)
	{
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid())
			{
				UE_LOG(LogOnline, Log, TEXT("Ending session %s on client"), *ShooterPlayerState->SessionName.ToString() );
				Sessions->EndSession(ShooterPlayerState->SessionName);
			}
		}
	}
}

void AShooterPlayerController::HandleReturnToMainMenu()
{
	CleanupSessionOnReturnToMenu();
}

void AShooterPlayerController::ClientReturnToMainMenu_Implementation(const FString& InReturnReason)
{		
	//@HUD return to main menu

	// Clear the flag so we don't do normal end of round stuff next
	bGameEndedFrame = false;
}

/** Ends and/or destroys game session */
void AShooterPlayerController::CleanupSessionOnReturnToMenu()
{
	//@HUD return to main menu
}

void AShooterPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	//called on clients
	OnPlayerStateReplicated();
}

void AShooterPlayerController::ClientGameEnded_Implementation(class AActor* EndGameFocus, bool bIsWinner)
{
	Super::ClientGameEnded_Implementation(EndGameFocus, bIsWinner);
	
	NotifyGameEnded();

	// Disable controls now the game has ended
	//SetCinematicMode(true, false, false, true, true);

	//bAllowGameActions = false;

	// Make sure that we still have valid view target
	//SetViewTarget(GetPawn());

	//@HUD: show win/lost screen

	// Flag that the game has just ended (if it's ended due to host loss we want to wait for ClientReturnToMainMenu_Implementation first, incase we don't want to process)
	bGameEndedFrame = true;

	ClientSetSpectatorCamera(EndGameFocus);
}

void AShooterPlayerController::ClientSendRoundEndEvent_Implementation(bool bIsWinner, int32 ExpendedTimeInSeconds)
{
	const auto Events = Online::GetEventsInterface();
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);

	if(bHasSentStartEvents && LocalPlayer != nullptr && Events.IsValid())
	{	
		auto UniqueId = LocalPlayer->GetPreferredUniqueNetId();

		if (UniqueId.IsValid())
		{
			FString MapName = *FPackageName::GetShortName(GetWorld()->PersistentLevel->GetOutermost()->GetName());
			AShooterPlayerState* ShooterPlayerState = GetPlayerState<AShooterPlayerState>();
			int32 PlayerScore = ShooterPlayerState ? ShooterPlayerState->GetScore() : 0;
			
			// Fire session end event for all cases
			FOnlineEventParms Params;
			Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
			Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused
			Params.Add( TEXT( "ExitStatusId" ), FVariantData( (int32)0 ) ); // unused
			Params.Add( TEXT( "PlayerScore" ), FVariantData( (int32)PlayerScore ) );
			Params.Add( TEXT( "PlayerWon" ), FVariantData( (bool)bIsWinner ) );
			Params.Add( TEXT( "MapName" ), FVariantData( MapName ) );
			Params.Add( TEXT( "MapNameString" ), FVariantData( MapName ) ); // @todo workaround for a bug in backend service, remove when fixed
			
			Events->TriggerEvent(*UniqueId, TEXT("PlayerSessionEnd"), Params);

		}
		bHasSentStartEvents = false;
	}
}

void AShooterPlayerController::SetCinematicMode(bool bInCinematicMode, bool bHidePlayer, bool bAffectsHUD, bool bAffectsMovement, bool bAffectsTurning)
{
	Super::SetCinematicMode(bInCinematicMode, bHidePlayer, bAffectsHUD, bAffectsMovement, bAffectsTurning);
	
	AShooterCharacter* MyPawn = Cast<AShooterCharacter>(GetPawn());
	// If we have a pawn we need to determine if we should show/hide the weapon
	AShooterWeapon* MyWeapon = MyPawn ? MyPawn->GetWeapon() : NULL;
	if (MyWeapon)
	{
		if (bInCinematicMode && bHidePlayer)
		{
			MyWeapon->SetActorHiddenInGame(true);
		}
		else if (!bCinematicMode)
		{
			MyWeapon->SetActorHiddenInGame(false);
		}
	}
}

bool AShooterPlayerController::IsMoveInputIgnored() const
{
	if (IsInState(NAME_Spectating))
	{
		return false;
	}
	else
	{
		return Super::IsMoveInputIgnored();
	}
}

bool AShooterPlayerController::IsLookInputIgnored() const
{
	if (IsInState(NAME_Spectating))
	{
		return false;
	}
	else
	{
		return Super::IsLookInputIgnored();
	}
}

void AShooterPlayerController::InitInputSystem()
{
	Super::InitInputSystem();

	UShooterPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
//		PersistentUser->TellInputAboutKeybindings();
	}
}

void AShooterPlayerController::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME_CONDITION( AShooterPlayerController, bInfiniteAmmo, COND_OwnerOnly );
	DOREPLIFETIME_CONDITION( AShooterPlayerController, bInfiniteClip, COND_OwnerOnly );
}

void AShooterPlayerController::Suicide()
{
	if ( IsInState(NAME_Playing) )
	{
		ServerSuicide();
	}
}

bool AShooterPlayerController::ServerSuicide_Validate()
{
	return true;
}

void AShooterPlayerController::ServerSuicide_Implementation()
{
	if ( (GetPawn() != NULL) && ((GetWorld()->TimeSeconds - GetPawn()->CreationTime > 1) || (GetNetMode() == NM_Standalone)) )
	{
		AShooterCharacter* MyPawn = Cast<AShooterCharacter>(GetPawn());
		if (MyPawn)
		{
			MyPawn->Suicide();
		}
	}
}

bool AShooterPlayerController::HasInfiniteAmmo() const
{
	return bInfiniteAmmo;
}

bool AShooterPlayerController::HasInfiniteClip() const
{
	return bInfiniteClip;
}

bool AShooterPlayerController::HasHealthRegen() const
{
	return bHealthRegen;
}

bool AShooterPlayerController::HasGodMode() const
{
	return bGodMode;
}

bool AShooterPlayerController::IsGameInputAllowed() const
{
	return bAllowGameActions && !bCinematicMode;
}

void AShooterPlayerController::Say( const FString& Msg )
{
	ServerSay(Msg.Left(128));
}

bool AShooterPlayerController::ServerSay_Validate( const FString& Msg )
{
	return true;
}

void AShooterPlayerController::ServerSay_Implementation( const FString& Msg )
{
	GetWorld()->GetAuthGameMode<AShooterGameMode>()->Broadcast(this, Msg, ServerSayString);
}


UShooterPersistentUser* AShooterPlayerController::GetPersistentUser() const
{
	UShooterLocalPlayer* const ShooterLP = Cast<UShooterLocalPlayer>(Player);
	return ShooterLP ? ShooterLP->GetPersistentUser() : NULL;
}

bool AShooterPlayerController::SetPause(bool bPause, FCanUnpause CanUnpauseDelegate /*= FCanUnpause()*/)
{
	const bool Result = APlayerController::SetPause(bPause, CanUnpauseDelegate);

	// Update rich presence.
	const auto PresenceInterface = Online::GetPresenceInterface();
	const auto Events = Online::GetEventsInterface();
	const auto LocalPlayer = Cast<ULocalPlayer>(Player);
	FUniqueNetIdRepl UserId = LocalPlayer->GetCachedUniqueNetId();

	if(PresenceInterface.IsValid() && UserId.IsValid())
	{
		FPresenceProperties Props;
		FOnlineUserPresenceStatus Status;
		Status.State = EOnlinePresenceState::Online;
		if(Result && bPause)
		{
			Status.StatusStr = FString("Paused");
			Props.Add(DefaultPresenceKey, FString("Paused"));
		}
		else
		{
			Status.StatusStr = FString("In Game");
			Props.Add(DefaultPresenceKey, FString("InGame"));
		}
		Status.Properties = Props;
		
		PresenceInterface->SetPresence(*UserId, Status);
	}

	// Don't send pause events while online since the game doesn't actually pause
	if(GetNetMode() == NM_Standalone && Events.IsValid() && PlayerState->GetUniqueId().IsValid())
	{
		FOnlineEventParms Params;
		Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
		Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused
		if(Result && bPause)
		{
			Events->TriggerEvent(*PlayerState->GetUniqueId(), TEXT("PlayerSessionPause"), Params);
		}
		else
		{
			Events->TriggerEvent(*PlayerState->GetUniqueId(), TEXT("PlayerSessionResume"), Params);
		}
	}

	return Result;
}

void AShooterPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UShooterPersistentUser* const PersistentUser = GetPersistentUser();
	if (PersistentUser)
	{
		PersistentUser->SaveIfDirty();
	}
	Super::EndPlay(EndPlayReason);
}

void AShooterPlayerController::SetZoomLevel(float Zoom, float MinFOV)
{
	Zoom = FMath::Clamp(Zoom, 0.0f, 1.0f);
	DesiredFOV = FMath::Lerp(MinFOV, DefaultFOV, 1.0f - Zoom);
}

float AShooterPlayerController::GetDesiredFOV() const
{
	return DesiredFOV;
}

void AShooterPlayerController::UpdateAchievementsOnGameEnd()
{
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer)
	{
		AShooterPlayerState* ShooterPlayerState = GetPlayerState<AShooterPlayerState>();
		if (ShooterPlayerState)
		{			
			const UShooterPersistentUser*  PersistentUser = GetPersistentUser();

			if (PersistentUser)
			{						
				const int32 Wins = PersistentUser->GetWins();
				const int32 Losses = PersistentUser->GetLosses();
				const int32 Matches = Wins + Losses;

				const int32 TotalKills = PersistentUser->GetKills();
				const int32 MatchScore = (int32)ShooterPlayerState->GetScore();

				float TotalGameAchievement = 0;
				float CurrentGameAchievement = 0;
			
				///////////////////////////////////////
				// Kill achievements
				if (TotalKills >= 1)
				{
					CurrentGameAchievement += 100.0f;
				}
				TotalGameAchievement += 100;

				{
					float fSomeKillPct = ((float)TotalKills / (float)SomeKillsCount) * 100.0f;
					fSomeKillPct = FMath::RoundToFloat(fSomeKillPct);
					UpdateAchievementProgress(ACH_SOME_KILLS, fSomeKillPct);

					CurrentGameAchievement += FMath::Min(fSomeKillPct, 100.0f);
					TotalGameAchievement += 100;
				}

				{
					float fLotsKillPct = ((float)TotalKills / (float)LotsKillsCount) * 100.0f;
					fLotsKillPct = FMath::RoundToFloat(fLotsKillPct);
					UpdateAchievementProgress(ACH_LOTS_KILLS, fLotsKillPct);

					CurrentGameAchievement += FMath::Min(fLotsKillPct, 100.0f);
					TotalGameAchievement += 100;
				}
				///////////////////////////////////////

				///////////////////////////////////////
				// Match Achievements
				{
					UpdateAchievementProgress(ACH_FINISH_MATCH, 100.0f);

					CurrentGameAchievement += 100;
					TotalGameAchievement += 100;
				}
			
				{
					float fLotsRoundsPct = ((float)Matches / (float)LotsMatchesCount) * 100.0f;
					fLotsRoundsPct = FMath::RoundToFloat(fLotsRoundsPct);
					UpdateAchievementProgress(ACH_LOTS_MATCHES, fLotsRoundsPct);

					CurrentGameAchievement += FMath::Min(fLotsRoundsPct, 100.0f);
					TotalGameAchievement += 100;
				}
				///////////////////////////////////////

				///////////////////////////////////////
				// Win Achievements
				if (Wins >= 1)
				{
					UpdateAchievementProgress(ACH_FIRST_WIN, 100.0f);

					CurrentGameAchievement += 100.0f;
				}
				TotalGameAchievement += 100;

				{			
					float fLotsWinPct = ((float)Wins / (float)LotsWinsCount) * 100.0f;
					fLotsWinPct = FMath::RoundToInt(fLotsWinPct);
					UpdateAchievementProgress(ACH_LOTS_WIN, fLotsWinPct);

					CurrentGameAchievement += FMath::Min(fLotsWinPct, 100.0f);
					TotalGameAchievement += 100;
				}

				{			
					float fManyWinPct = ((float)Wins / (float)ManyWinsCount) * 100.0f;
					fManyWinPct = FMath::RoundToInt(fManyWinPct);
					UpdateAchievementProgress(ACH_MANY_WIN, fManyWinPct);

					CurrentGameAchievement += FMath::Min(fManyWinPct, 100.0f);
					TotalGameAchievement += 100;
				}
				///////////////////////////////////////

				///////////////////////////////////////

				///////////////////////////////////////
				// Score Achievements
				{
					float fGoodScorePct = ((float)MatchScore / (float)GoodScoreCount) * 100.0f;
					fGoodScorePct = FMath::RoundToFloat(fGoodScorePct);
					UpdateAchievementProgress(ACH_GOOD_SCORE, fGoodScorePct);
				}

				{
					float fGreatScorePct = ((float)MatchScore / (float)GreatScoreCount) * 100.0f;
					fGreatScorePct = FMath::RoundToFloat(fGreatScorePct);
					UpdateAchievementProgress(ACH_GREAT_SCORE, fGreatScorePct);
				}
				///////////////////////////////////////

				///////////////////////////////////////
				// Map Play Achievements
				UWorld* World = GetWorld();
				if (World)
				{			
					FString MapName = *FPackageName::GetShortName(World->PersistentLevel->GetOutermost()->GetName());
					if (MapName.Find(TEXT("Highrise")) != -1)
					{
						UpdateAchievementProgress(ACH_PLAY_HIGHRISE, 100.0f);
					}
					else if (MapName.Find(TEXT("Sanctuary")) != -1)
					{
						UpdateAchievementProgress(ACH_PLAY_SANCTUARY, 100.0f);
					}
				}
				///////////////////////////////////////			

				const auto Events = Online::GetEventsInterface();
				const auto Identity = Online::GetIdentityInterface();

				if (Events.IsValid() && Identity.IsValid())
				{							
					int32 UserIndex = LocalPlayer->GetControllerId();
					TSharedPtr<const FUniqueNetId> UniqueID = Identity->GetUniquePlayerId(UserIndex);
					if (UniqueID.IsValid())
					{				
						FOnlineEventParms Params;

						float fGamePct = (CurrentGameAchievement / TotalGameAchievement) * 100.0f;
						fGamePct = FMath::RoundToFloat(fGamePct);
						Params.Add( TEXT( "CompletionPercent" ), FVariantData( (float)fGamePct ) );
						if (UniqueID.IsValid())
						{				
							Events->TriggerEvent(*UniqueID, TEXT("GameProgress"), Params);
						}
					}
				}
			}
		}
	}
}

void AShooterPlayerController::SetPlayerName(const FString& NewName)
{
	SetName(NewName.Left(32));
}

void AShooterPlayerController::InitPlayerState()
{
	Super::InitPlayerState();
	//called on server
	OnPlayerStateReplicated();
}

bool AShooterPlayerController::IsEnemyFor(AController* TestPC) const
{
	if (TestPC == this || TestPC == NULL)
	{
		return false;
	}

	AShooterPlayerState* TestPlayerState = TestPC->GetPlayerState<AShooterPlayerState>();
	AShooterPlayerState* MyPlayerState = GetPlayerState<AShooterPlayerState>();

	if (TestPC->GetWorld()->GetGameState() && GetWorld()->GetGameState()->GameModeClass)
	{
		const AShooterGameMode* DefGame = GetWorld()->GetGameState()->GameModeClass->GetDefaultObject<AShooterGameMode>();
		if (DefGame && MyPlayerState && TestPlayerState)
		{
			return DefGame->CanDealDamage(TestPlayerState, MyPlayerState);
		}
	}
	return true;
}

void AShooterPlayerController::ToggleSpeaking(bool bSpeaking)
{
	return;
	UShooterGameUserSettings* UserSettings = CastChecked<UShooterGameUserSettings>(GEngine->GetGameUserSettings());

	ULocalPlayer* LP = Cast<ULocalPlayer>(Player);
	if (LP != NULL)
	{
		IOnlineVoicePtr VoiceInt = Online::GetVoiceInterface();
		if (VoiceInt.IsValid())
		{
			if (bSpeaking)
			{
				VoiceInt->StartNetworkedVoice(LP->GetControllerId());
			}
			else
			{
				VoiceInt->StopNetworkedVoice(LP->GetControllerId());
			}
		}
	}
}

void AShooterPlayerController::UpdateChatOption()
{
	return;
	UShooterGameUserSettings* UserSettings = CastChecked<UShooterGameUserSettings>(GEngine->GetGameUserSettings());

	ULocalPlayer* LP = Cast<ULocalPlayer>(Player);
	if (LP != NULL)
	{
		IOnlineVoicePtr VoiceInt = Online::GetVoiceInterface();
		if (VoiceInt.IsValid())
		{
			if (UserSettings->IsVoiceChatEnabled())
			{
				VoiceInt->RegisterLocalTalker(LP->GetControllerId());
				NumberOfTalkers = 0;
				VoiceInt->OnPlayerTalkingStateChangedDelegates.AddUObject(this, &AShooterPlayerController::TalkingStateChanged);
			}
			else
			{
				VoiceInt->UnregisterLocalTalker(LP->GetControllerId());
				//VoiceInt->OnPlayerTalkingStateChangedDelegates.RemoveUObject(this, &AShooterPlayerController::TalkingStateChanged);
				SetSoundClassVolume(TEXT("Master"), UserSettings->AudioMasterVolume);
			}
		}
	}
}

void AShooterPlayerController::ReceivedGameModeClass(TSubclassOf<class AGameModeBase> GameModeClass)
{
	Super::ReceivedGameModeClass(GameModeClass);
}

void AShooterPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);
	//called on owning client
	OnPossessed(P);
}

void AShooterPlayerController::SetSoundClassVolume(FString ClassName, float NewVolume)
{
	UShooterGameUserSettings* UserSettings = CastChecked<UShooterGameUserSettings>(GEngine->GetGameUserSettings());
	UserSettings->ChangeSoundClassVolume(ClassName, NewVolume);
}

void AShooterPlayerController::ClientSendMessage_Implementation(EMessageTypes::Type MessageType, bool bRelativeToMe, const FString& InstigatorName, const FString& InstigatedName, uint8 OptionalRank, uint8 OptionalTeam)
{
	/*AShooterGameState* GS = Cast<AShooterGameState>(GetWorld()->GetGameState());
	FGameMessage TheMessage = GS->GetGameMessage(MessageType, OptionalRank);
	if (TheMessage.Sound)
	{
		UGameplayStatics::SpawnSoundAttached(TheMessage.Sound, GetPawn() ? GetPawn()->GetRootComponent() : GetRootComponent());
	}
	AShooterHUD* ShooterHUD = GetShooterHUD();
	if (ShooterHUD)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Instigator"), FText::FromString( InstigatorName ));
		Arguments.Add(TEXT("Instigated"), FText::FromString( InstigatedName ));
		Arguments.Add(TEXT("TeamColor"), GTeamColorsText[OptionalTeam]);
		FText TheText;
		if (bRelativeToMe)
		{
			TheText = FText::Format(TheMessage.LocalMessageText, Arguments);
		}
		else
		{
			TheText = FText::Format(TheMessage.RemoteMessageText, Arguments);			
		}

		if (MessageType < EMessageTypes::NUM_ACHIEVEMENTS)
		{
			ShooterHUD->AddGameAchievement(TheText);
			NotifyMessageReceived(TheText);
		}
		else if (MessageType < EMessageTypes::NUM_CTF)
		{
			ShooterHUD->AddGameMessage(TheText, GTeamColors[OptionalTeam]);
			NotifyMessageReceived(TheText, GTeamColors[OptionalTeam]);
		}
	}*/
}

void AShooterPlayerController::TalkingStateChanged(TSharedRef<const FUniqueNetId> TalkerId, bool bIsTalking)
{
	const int32 PreviousNumberOfTalkers = NumberOfTalkers;
	if (bIsTalking)
	{
		NumberOfTalkers++;
	}
	else
	{
		NumberOfTalkers--;
	}
	NumberOfTalkers = FMath::Max(NumberOfTalkers, 0);
	UShooterGameUserSettings* UserSettings = CastChecked<UShooterGameUserSettings>(GEngine->GetGameUserSettings());
	if (NumberOfTalkers == 1 && PreviousNumberOfTalkers == 0)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("talking"));
		UserSettings->ChangeSoundClassVolume("Master", UserSettings->AudioMasterVolume * 0.2f);
	}
	else if (NumberOfTalkers == 0)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("not talking"));
		UserSettings->ChangeSoundClassVolume("Master", UserSettings->AudioMasterVolume);
	}
}

bool AShooterPlayerController::ClientLoadStreamingLevel_Validate(FName LevelPath, int32 UUID)
{
	return true;
}

void AShooterPlayerController::ClientLoadStreamingLevel_Implementation(FName LevelPath, int32 UUID)
{
	FLatentActionInfo info;
	info.UUID = UUID;
	UGameplayStatics::LoadStreamLevel(this, LevelPath, true, true, info);
}

void AShooterPlayerController::PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{
	Super::PreClientTravel( PendingURL, TravelType, bIsSeamlessTravel );

	//@HUD: show loading screen, hide scoreboard
}

void AShooterPlayerController::AddLocalPlayer()
{
	FString Error;
	ULocalPlayer* LocalPlayer = GetWorld()->GetGameInstance()->CreateLocalPlayer(-1, Error, true);
}

void AShooterPlayerController::RemoveLocalPlayer()
{
	FString Error;
	ULocalPlayer* LastLocalPlayer = GetWorld()->GetGameInstance()->GetLocalPlayerByIndex(GetWorld()->GetGameInstance()->GetNumLocalPlayers()-1);
	if (LastLocalPlayer->PlayerController)
	{
		AShooterCharacter* RemovedPawn = Cast<AShooterCharacter>(LastLocalPlayer->PlayerController->GetPawn());
		if (RemovedPawn)
		{
			RemovedPawn->Suicide();
		}
	}
	GetWorld()->GetGameInstance()->RemoveLocalPlayer(LastLocalPlayer);
}

void AShooterPlayerController::SetMousePosition(int32 x, int32 y)
{
	FViewport* Viewport = CastChecked<ULocalPlayer>(Player)->ViewportClient->Viewport;
	Viewport->SetMouse(x,y);
}

void AShooterPlayerController::SetMousePositionToCenter()
{
	FViewport* Viewport = CastChecked<ULocalPlayer>(Player)->ViewportClient->Viewport;
	FIntPoint ViewportSize = Viewport->GetSizeXY();
	Viewport->SetMouse(ViewportSize.X/2, ViewportSize.Y/2);
}

void AShooterPlayerController::SetMouseSensitivityX(float Sensitivity)
{
	PlayerInput->SetMouseSensitivity(Sensitivity, PlayerInput->GetMouseSensitivityY());
}

void AShooterPlayerController::SetMouseSensitivityY(float Sensitivity)
{
	PlayerInput->SetMouseSensitivity(PlayerInput->GetMouseSensitivityX(), Sensitivity);
}


float AShooterPlayerController::GetMouseSensitivityX() const
{
	return PlayerInput->GetMouseSensitivityX();
}

float AShooterPlayerController::GetMouseSensitivityY() const
{
	return PlayerInput->GetMouseSensitivityY();
}
