// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "GameRules/ShooterGameMode.h"
#include "Player/ShooterSpectatorPawn.h"
#include "Player/ShooterPlayerController.h"
#include "Player/ShooterPlayerState.h"
#include "ShooterGameUserSettings.h"
#include "GameRules/ShooterGameState.h"
#include "GameRules/ShooterGameSession.h"
#include "AI/ShooterAIController.h"
#include "AI/ShooterMonsterController.h"
#include "ShooterTeamStart.h"
#include "ShooterGameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/PlayerStartPIE.h"
#include "UObject/ConstructorHelpers.h"
#include "Player/ShooterCharacter.h"
#include "Components/CapsuleComponent.h"

DEFINE_LOG_CATEGORY(LogShooterGameMode);

AShooterGameMode::AShooterGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<APawn> BotPawnOb(TEXT("/Game/Blueprints/Pawns/BotPawn"));
	BotPawnClass = BotPawnOb.Class;

	static ConstructorHelpers::FClassFinder<APlayerController> PCOb(TEXT("/Game/Blueprints/Game/ShooterPlayerController_BP"));
	PlayerControllerClass = PCOb.Class;
	
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnOb(TEXT("/Game/Blueprints/Pawns/PlayerPawn"));
	DefaultPawnClass = PlayerPawnOb.Class;

	PlayerStateClass = AShooterPlayerState::StaticClass();
	SpectatorClass = AShooterSpectatorPawn::StaticClass();
	GameStateClass = AShooterGameState::StaticClass();

	bDelayedStart = false;
	MinRespawnDelay = 5.0f;

	bNeedsBotCreation = true;
	bUseSeamlessTravel = true;

	GameModeInfo.GameModeName = NSLOCTEXT("Game", "UndefinedGameMode", "Undefined Game Mode");
	GameModeInfo.GameClassName = GetClass()->GetName();
	GameModeInfo.MinTeams=0;
	GameModeInfo.MaxTeams=0;
	GameModeInfo.bAddToMenu=false;
	GameModeInfo.bRequiresMapPrefix=false;
	
	bUnlimitedRoundTime = RoundTime == 0;
}

void AShooterGameMode::PostInitProperties()
{
	Super::PostInitProperties();
	GameModeInfo.GameModePrefix = GetGameModeShortName();
}

FString AShooterGameMode::GetBotsCountOptionName()
{
	return FString(TEXT("Bots"));
}

void AShooterGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	UserSettings = CastChecked<UShooterGameUserSettings>(GEngine->GetGameUserSettings());
	const int32 BotsCountOptionValue = UGameplayStatics::GetIntOption(Options, GetBotsCountOptionName(), 0);
	ScoreLimit = UGameplayStatics::GetIntOption(Options, TEXT("ScoreLimit"), ScoreLimit);
	RoundTime = UGameplayStatics::GetIntOption(Options, TEXT("RoundTime"), RoundTime);
	NumTeams = UGameplayStatics::GetIntOption(Options, TEXT("NumTeams"), NumTeams);
	SetAllowBots(BotsCountOptionValue > 0 ? true : false, BotsCountOptionValue);

	GetWorldTimerManager().SetTimer(DefaultTimerHandle, this, &AShooterGameMode::DefaultTimer, 1.0f, true,-1.0f);	

	//save score limit, round time, etc. into ini file
	GEngine->GetGameUserSettings()->SaveSettings();

	Super::InitGame(MapName, Options, ErrorMessage);
}

void AShooterGameMode::InitGameState()
{
	Super::InitGameState();
	ShooterGameState = CastChecked<AShooterGameState>(GameState);
	/*ShooterGameState->bClientSideHitVerification = bClientSideHitVerification;
	ShooterGameState->bReplicateProjectiles = bReplicateProjectiles;*/
}

void AShooterGameMode::SetAllowBots(bool bInAllowBots, int32 InMaxBots)
{
	MaxBots = InMaxBots;
}

/** Returns game session class to use */
TSubclassOf<AGameSession> AShooterGameMode::GetGameSessionClass() const
{
	return AShooterGameSession::StaticClass();
}

void AShooterGameMode::HandleMatchIsWaitingToStart()
{
	if (bNeedsBotCreation)
	{
		CreateBotControllers();
		AssignBotsColors();
		bNeedsBotCreation = false;
	}

	if (bDelayedStart)
	{
		// start warmup if needed
		if (ShooterGameState && ShooterGameState->RemainingTime == 0)
		{
			const bool bWantsMatchWarmup = !GetWorld()->IsPlayInEditor();
			if (bWantsMatchWarmup && WarmupTime > 0)
			{
				ShooterGameState->RemainingTime = WarmupTime;
			}
			else
			{
				ShooterGameState->RemainingTime = 0.0f;
			}
		}
	}
}

void AShooterGameMode::HandleMatchHasStarted()
{
	bNeedsBotCreation = true;
	Super::HandleMatchHasStarted();
	
	ShooterGameState->RemainingTime = RoundTime;	
	StartBots();

	// notify players
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		AShooterPlayerController* PC = Cast<AShooterPlayerController>(It->Get());
		if (PC)
		{
			PC->ClientGameStarted();
		}
	}
}

void AShooterGameMode::FinishMatch()
{
	if (IsMatchInProgress())
	{
		EndMatch();
		AActor* FocusActor = DetermineMatchWinner();		

		// notify players
		for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			AShooterPlayerState* PlayerState = (It->Get())->GetPlayerState<AShooterPlayerState>();
			const bool bIsWinner = IsWinner(PlayerState);
			
			//stop shooting and unpossess, to force 3rd person view
			AShooterCharacter* Character = Cast<AShooterCharacter>((It->Get())->GetPawn());
			if (Character)
			{
				Character->StopAllWeaponFire();
			}
			if ((It->Get())->IsA(PlayerControllerClass))
			{
				//Unpossess to switch to third person view
				(It->Get())->UnPossess();
			}
			(It->Get())->GameHasEnded(FocusActor, bIsWinner);
		}

		// lock all pawns
		// pawns are not marked as keep for seamless travel, so we will create new pawns on the next match rather than
		// turning these back on.
		for (TActorIterator<AShooterCharacter> It(GetWorld()); It; ++It)
		{
			(*It)->TurnOff();
		}

		// set up to restart the match
		ShooterGameState->RemainingTime = TimeBetweenMatches;
	}
}

void AShooterGameMode::RequestFinishAndExitToMainMenu()
{
	FText RemoteReturnReason = NSLOCTEXT("NetworkErrors", "HostHasLeft", "Host has left the game.");
	
	FinishMatch();

	//@GI: RemoveSplitScreenPlayers

	AShooterPlayerController* LocalPrimaryController = nullptr;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AShooterPlayerController* Controller = Cast<AShooterPlayerController>(*Iterator);
		if (Controller && !Controller->IsLocalController())
		{
			Controller->ClientReturnToMainMenuWithTextReason(RemoteReturnReason);
		}
		else
		{
			LocalPrimaryController = Controller;
		}
	}

	// GameInstance should be calling this from an EndState.  So call the PC function that performs cleanup, not the one that sets GI state.
	if (LocalPrimaryController != NULL)
	{
		LocalPrimaryController->HandleReturnToMainMenu();
	}
}

AActor* AShooterGameMode::DetermineMatchWinner()
{
	// nothing to do here
	return NULL;
}

bool AShooterGameMode::IsWinner(class AShooterPlayerState* PlayerState) const
{
	return false;
}

void AShooterGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	const bool bMatchIsOver = ShooterGameState && ShooterGameState->HasMatchEnded();
	if( bMatchIsOver )
	{
		ErrorMessage = TEXT("Match is over!");
	}
	else
	{
		// GameSession can be NULL if the match is over
		Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	}
}


void AShooterGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// update spectator location for client
	AShooterPlayerController* NewPC = Cast<AShooterPlayerController>(NewPlayer);
	if (NewPC && NewPC->GetPawn() == NULL)
	{
		NewPC->ClientSetSpectatorCamera();
	}
	
	// notify new player if match is already in progress
	if (NewPC && IsMatchInProgress())
	{
		NewPC->ClientGameStarted();
		NewPC->ClientStartOnlineGame();
	}
}

bool AShooterGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return ShooterGameState && (ShooterGameState->GetMatchState() == MatchState::InProgress || ShooterGameState->GetMatchState() == MatchState::WaitingToStart);
}

void AShooterGameMode::Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, TSubclassOf<class AShooterWeapon> KillerWeaponClass, TSubclassOf<class UShooterDamageType> KillerDmgType)
{
	AShooterPlayerState* KillerPlayerState = Killer ? Killer->GetPlayerState<AShooterPlayerState>() : NULL;
	AShooterPlayerState* VictimPlayerState = KilledPlayer ? KilledPlayer->GetPlayerState<AShooterPlayerState>() : NULL;

	if (KillerPlayerState && KillerPlayerState != VictimPlayerState)
	{
		KillerPlayerState->ScoreKill(VictimPlayerState, KillScore);
		KillerPlayerState->InformAboutKill(KillerPlayerState, VictimPlayerState);
		AShooterCharacter* KillerPawn = Cast<AShooterCharacter>(Killer->GetPawn());
		AShooterWeapon* KillerWeapon = KillerPawn ? KillerPawn->GetWeapon() : NULL;
		if (KillerWeapon)
		{
			KillerWeapon->KilledEvent(KilledPawn, KilledPlayer, KillerWeaponClass, KillerDmgType);
		}
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->ScoreDeath(KillerPlayerState, DeathScore);
		VictimPlayerState->BroadcastDeath(KillerPlayerState, KillerWeaponClass, KillerDmgType);
		VictimPlayerState->UseLife();
	}

	if (VictimPlayerState != NULL && (KillerPlayerState == NULL || KillerPlayerState == VictimPlayerState) && Killer && !Killer->IsA(AShooterMonsterController::StaticClass()) )
	{
		VictimPlayerState->ScoreSuicide(SuicideScore);
	}
}

void AShooterGameMode::CheckAndNotifyAchievements(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, class AActor* DamageCauser)
{
	UShooterMessageHandler* MessageHandlerInst = ShooterGameState->GetMessageHandler();
	if (!MessageHandlerInst || !MessageHandlerInst->IsValidLowLevel())
	{
		return;
	}
	const bool bWasSuicide = Killer == KilledPlayer;

	FString KillerName, VictimName;
	AShooterPlayerState* KillerPS = Killer ? Killer->GetPlayerState<AShooterPlayerState>() : NULL;
	if (KillerPS)
	{
		KillerName = KillerPS->GetPlayerName();	
	}
	else
	{
		//no player state -- can only be a non-bot NPC (a monster)
		KillerName = NSLOCTEXT("HUD", "AMonster", "A monster").ToString();
	}
	AShooterPlayerState* VictimPS = KilledPlayer ? KilledPlayer->GetPlayerState<AShooterPlayerState>() : NULL;
	if (VictimPS)
	{
		VictimName = VictimPS->GetPlayerName();
	}
	else
	{
		//no player state -- can only be a non-bot NPC (a monster)
		VictimName = NSLOCTEXT("HUD", "AMonster", "A monster").ToString();
	}
	
	//first blood?
	if (ShooterGameState->GetTotalKills() == 1 && !bWasSuicide)
	{
		MessagePlayers(EMessageTypes::FirstBlood, Killer, KillerName, VictimName);
	}

	AShooterCharacter* KilledChar = Cast<AShooterCharacter>(KilledPawn);
	if (KilledChar)
	{
		//headshot?
		AShooterWeapon* Weapon = Cast<AShooterWeapon>(DamageCauser);
		if (Weapon && Weapon->bRewardHeadshots && KilledChar->HeadBoneNames.Find(KilledChar->GetLastHitInfo().GetPointDamageEvent().HitInfo.BoneName) != INDEX_NONE)
		{
			MessagePlayers(EMessageTypes::Headshot, Killer, KillerName, VictimName);
		}
	}

	if (KillerPS && !bWasSuicide)
	{
		//multi kill?
		//-2 because two consecutive kills awards a double kill (the first achievement level, index 0)
		const int32 MultiKillIndex = FMath::Min(KillerPS->GetMultiKills() - 2, MessageHandlerInst->MultiKillMessages.Num() - 1);
		if (MultiKillIndex >= 0)
		{
			MessagePlayers(EMessageTypes::MultiKill, Killer, KillerName, VictimName, MultiKillIndex);
		}
		//killing spree?
		if (MessageHandlerInst->KillsForSpreeIncrease > 0 && KillerPS->GetKillsSinceLastDeath() >= MessageHandlerInst->KillsForSpreeIncrease && KillerPS->GetKillsSinceLastDeath() % MessageHandlerInst->KillsForSpreeIncrease == 0)
		{
			const int32 SpreeLevel = KillerPS->GetKillsSinceLastDeath() / MessageHandlerInst->KillsForSpreeIncrease - 1;
			MessagePlayers(EMessageTypes::KillingSpree, Killer, KillerName, VictimName, SpreeLevel);
		}
	}
	if (VictimPS)
	{
		if (MessageHandlerInst->KillsForSpreeIncrease > 0 && VictimPS->GetKillsSinceLastDeath() >= MessageHandlerInst->KillsForSpreeIncrease)
		{
			const int32 SpreeLevel = VictimPS->GetKillsSinceLastDeath() / MessageHandlerInst->KillsForSpreeIncrease - 1;
			MessagePlayers(EMessageTypes::KillingSpreeEnded, Killer, KillerName, VictimName, SpreeLevel);
		}
	}
}

float AShooterGameMode::ModifyDamage(float Damage, AActor* DamagedActor, AController* EventInstigator) const
{
	float ActualDamage = Damage;
	if (ShooterGameState->HasMatchEnded())
	{
		return 0.f;
	}

	AShooterCharacter* DamagedPawn = Cast<AShooterCharacter>(DamagedActor);
	if (DamagedPawn && EventInstigator)
	{
		AShooterPlayerState* DamagedPlayerState = DamagedPawn->GetPlayerState<AShooterPlayerState>();
		AShooterPlayerState* InstigatorPlayerState = EventInstigator->GetPlayerState<AShooterPlayerState>();

		// disable friendly fire
		if (!CanDealDamage(InstigatorPlayerState, DamagedPlayerState))
		{
			ActualDamage = 0.0f;
		}

		// scale self instigated damage
		if (InstigatorPlayerState == DamagedPlayerState)
		{
			ActualDamage *= DamageSelfScale;
		}
	}

	return ActualDamage;
}

bool AShooterGameMode::CanDealDamage(class AShooterPlayerState* DamageInstigator, class AShooterPlayerState* DamagedPlayer) const
{
	return true;
}

bool AShooterGameMode::AllowCheats(APlayerController* P)
{
	return true;
}

bool AShooterGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	return false;
}

/** returns default pawn class for given controller */
UClass* AShooterGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (Cast<AShooterAIController>(InController))
	{
		return BotPawnClass;
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

/** select best spawn point for player */
AActor* AShooterGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<APlayerStart*> PreferredSpawns;
	TArray<APlayerStart*> FallbackSpawns;
	APlayerStart* BestStart = NULL;

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* TestSpawn = *It;
		if (TestSpawn->IsA<APlayerStartPIE>())
		{
			// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
			BestStart = TestSpawn;
			break;
		}
		else
		{
			if (IsSpawnpointAllowed(TestSpawn, Player))
			{
				if (AnyPawnOverlapsSpawnPoint(TestSpawn, Player))
				{
					PreferredSpawns.Add(TestSpawn);
				}
				else
				{
					FallbackSpawns.Add(TestSpawn);
				}
			}
		}
	}
	
	if (BestStart == NULL)
	{
		if (PreferredSpawns.Num() > 0)
		{
			BestStart = GetBestSpawnPoint(PreferredSpawns, Player);
		}
		else if (FallbackSpawns.Num() > 0)
		{
			BestStart = GetBestSpawnPoint(FallbackSpawns, Player);
		}
	}

	return BestStart ? BestStart : Super::ChoosePlayerStart_Implementation(Player);
}

bool AShooterGameMode::IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const
{
	AShooterTeamStart* ShooterSpawnPoint = Cast<AShooterTeamStart>(SpawnPoint);
	if (ShooterSpawnPoint)
	{
		AShooterAIController* AIController = Cast<AShooterAIController>(Player);
		if (ShooterSpawnPoint->bNotForBots && AIController)
		{
			return false;
		}
		if (ShooterSpawnPoint->bNotForPlayers && AIController == NULL)
		{
			return false;
		}
	}
	return true;
}

bool AShooterGameMode::AnyPawnOverlapsSpawnPoint(APlayerStart* SpawnPoint, AController* Player) const
{
	ACharacter* MyPawn = Cast<ACharacter>(DefaultPawnClass.GetDefaultObject());
	if (MyPawn)
	{
		const FVector SpawnLocation = SpawnPoint->GetActorLocation();
		for (TActorIterator<AShooterCharacter> It(GetWorld()); It; ++It)
		{
			AShooterCharacter* OtherPawn = *It;
			if (OtherPawn && OtherPawn->IsAlive())
			{
				const float CombinedHeight = (MyPawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + OtherPawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()) * 2.0f;
				const float CombinedRadius = MyPawn->GetCapsuleComponent()->GetScaledCapsuleRadius() + OtherPawn->GetCapsuleComponent()->GetScaledCapsuleRadius();
				const FVector OtherLocation = OtherPawn->GetActorLocation();

				// check if player start overlaps this pawn
				if (FMath::Abs(SpawnLocation.Z - OtherLocation.Z) < CombinedHeight && (SpawnLocation - OtherLocation).Size2D() < CombinedRadius)
				{
					return false;
				}
			}
		}
	}
	return true;
}

APlayerStart* AShooterGameMode::GetBestSpawnPoint(const TArray<APlayerStart*>& AvailableSpawns, AController* Player) const
{
	if (AvailableSpawns.Num() == 0)
	{
		return NULL;
	}
	const int32 RandomAvailableIndex = FMath::RandHelper(AvailableSpawns.Num());
	return AvailableSpawns[RandomAvailableIndex];
}

void AShooterGameMode::CreateBotControllers()
{
	UWorld* World = GetWorld();
	int32 ExistingBots = 0;
	for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
	{		
		AShooterAIController* AIC = Cast<AShooterAIController>(It->Get());
		if (AIC)
		{
			++ExistingBots;
		}
	}

	// Create any necessary AIControllers.  Hold off on Pawn creation until pawns are actually necessary or need recreating.	
	int32 BotNum = ExistingBots;
	for (int32 i = 0; i < MaxBots - ExistingBots; ++i)
	{
		CreateBot(BotNum + i);
	}
}

void AShooterGameMode::AssignBotsColors()
{
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{		
		AShooterAIController* AIC = Cast<AShooterAIController>(It->Get());
		if (AIC)
		{
			AShooterPlayerState* AIPS = AIC->GetPlayerState<AShooterPlayerState>();
			if (AIPS)
			{
				AIPS->ServerSetColor(0, FLinearColor::MakeRandomColor());
				AIPS->ServerSetColor(1, FLinearColor::MakeRandomColor());
				AIPS->ServerSetColor(2, FLinearColor::MakeRandomColor());
			}
		}
	}
}

AShooterAIController* AShooterGameMode::CreateBot(int32 BotNum)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = nullptr;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnInfo.OverrideLevel = nullptr;

	UWorld* World = GetWorld();
	AShooterAIController* AIC = World->SpawnActor<AShooterAIController>(SpawnInfo);
	InitBot(AIC, BotNum);

	return AIC;
}

void AShooterGameMode::StartBots()
{
	UWorld* World = GetWorld();
	for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
	{		
		AShooterAIController* AIC = Cast<AShooterAIController>(It->Get());
		if (AIC)
		{
			RestartPlayer(AIC);
		}
	}	
}

void AShooterGameMode::InitBot(AShooterAIController* AIC, int32 BotNum)
{	
	if (AIC)
	{
		if (AIC->PlayerState)
		{
			FString BotName = FString::Printf(TEXT("Bot %d"), BotNum);
			AIC->PlayerState->SetPlayerName(BotName);
			AShooterPlayerState* AIPS = AIC->GetPlayerState<AShooterPlayerState>();
			if (AIPS)
			{
				AIPS->ServerSetColor(0, FLinearColor::MakeRandomColor());
				AIPS->ServerSetColor(1, FLinearColor::MakeRandomColor());
				AIPS->ServerSetColor(2, FLinearColor::MakeRandomColor());
			}
		}		
	}
}

void AShooterGameMode::Logout(AController* Exiting)
{
	AShooterCharacter* ExitingCharacter = Cast<AShooterCharacter>(Exiting->GetPawn());
	if (ExitingCharacter)
	{
		ExitingCharacter->PlayerLeftGame();
	}
	Super::Logout(Exiting);
}

void AShooterGameMode::SetPlayersLifes(int32 NewRemainingLifes)
{
	for (APlayerState* PS : GameState->PlayerArray)
	{
		AShooterPlayerState* APS = CastChecked<AShooterPlayerState>(PS);
		APS->SetLives(NewRemainingLifes);
	}
}

void AShooterGameMode::RestartPlayer(AController* NewPlayer)
{
	if (ShooterGameState && ShooterGameState->HasMatchEnded())
	{
		//do not respawn if match is over
		return;
	}
	AShooterPlayerState* SPS = NewPlayer->GetPlayerState<AShooterPlayerState>();
	if (SPS)
	{
		if (SPS->HasLivesRemaining())
		{
			SPS->RestartPlayer();
			Super::RestartPlayer(NewPlayer);
		}
	}
	else
	{
		Super::RestartPlayer(NewPlayer);
	}
}

void AShooterGameMode::RestartAllPlayers(bool bDeadOnly)
{
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		if (!bDeadOnly)
		{
			RestartPlayer(It->Get());
		}
		else
		{
			AShooterCharacter* Char = Cast<AShooterCharacter>((It->Get())->GetPawn());
			if (!Char || !Char->IsAlive())
			{
				RestartPlayer(It->Get());
			}
		}
	}
}

void AShooterGameMode::RestartGame()
{
	Super::RestartGame();
}

bool AShooterGameMode::AnyPlayerHasLivesRemaining() const
{
	for (APlayerState* PS : GameState->PlayerArray)
	{
		AShooterPlayerState* APS = CastChecked<AShooterPlayerState>(PS);
		if (APS->HasLivesRemaining())
		{
			return true;
		}
	}
	return false;
}

void AShooterGameMode::MessagePlayers(TEnumAsByte<EMessageTypes::Type> MessageType, AController* MessageRelativeTo, const FString& InstigatorName, const FString& InstigatedName, uint8 OptionalRank, uint8 OptionalTeam) const
{
	FGameMessage TheMessage = ShooterGameState->GetGameMessage(MessageType, OptionalRank);
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{	
		AShooterPlayerController* PC = Cast<AShooterPlayerController>(It->Get());
		if (PC)
		{
			if (PC != MessageRelativeTo && !TheMessage.RemoteMessageText.IsEmpty())
			{
				PC->ClientSendMessage(MessageType, false, InstigatorName,InstigatedName, OptionalRank, OptionalTeam);
			}
			else if (PC == MessageRelativeTo && !TheMessage.LocalMessageText.IsEmpty())
			{
				PC->ClientSendMessage(MessageType, true, InstigatorName,InstigatedName, OptionalRank, OptionalTeam);
			}
		}
	}
}

void AShooterGameMode::MessagePlayers(TEnumAsByte<EMessageTypes::Type> MessageType, AController* MessageRelativeTo, const APawn* MessageInstigator /*= NULL*/, const APawn* Instigated /*= NULL*/, uint8 OptionalRank /*= 0*/, uint8 OptionalTeam /*= 0*/) const
{
	FString InstigatorName, InstigatedName;
	const AShooterPlayerState* InstigatorPS = MessageInstigator ? MessageInstigator->GetPlayerState<AShooterPlayerState>() : NULL;
	if (InstigatorPS)
	{
		InstigatorName = InstigatorPS->GetPlayerName();
	}
	else
	{
		InstigatorName = NSLOCTEXT("HUD", "AMonster", "A monster").ToString();
	}
	const AShooterPlayerState* InstigatedPS = Instigated ? Instigated->GetPlayerState<AShooterPlayerState>() : NULL;
	if (InstigatedPS)
	{
		InstigatedName = InstigatedPS->GetPlayerName();
	}
	else
	{
		InstigatedName = NSLOCTEXT("HUD", "AMonster", "A monster").ToString();
	}
	MessagePlayers(MessageType, MessageRelativeTo, InstigatorName, InstigatedName, OptionalRank, OptionalTeam);
}

FString AShooterGameMode::GetGameModeShortName() const
{
	FString AliasName, UnusedStr;
	const FString& MyClassName = GetClass()->GetName();
	int32 const NumAliases = GameModeList.Num();
	for (int32 Idx=0; Idx<NumAliases; ++Idx)
	{
		const FGameModeInfo& Alias = GameModeList[Idx];
		Alias.GameClassName.Split(TEXT("."), &UnusedStr, &AliasName);
		if (AliasName == MyClassName)
		{
			return Alias.GameModePrefix;
		}
	}
	return TEXT("");
}
