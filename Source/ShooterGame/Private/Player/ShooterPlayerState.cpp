// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "Player/ShooterPlayerState.h"
#include "Player/ShooterPlayerController.h"
#include "Player/ShooterCharacter.h"
#include "Player/ShooterLocalPlayer.h"
#include "Player/ShooterPersistentUser.h"
#include "Player/ShooterSpectatorPawn.h"
#include "ShooterGameInstance.h"
#include "GameRules/ShooterGameState.h"
#include "GameRules/ShooterGameMode.h"
#include "Net/UnrealNetwork.h"
#include "AI/ShooterAIController.h"

AShooterPlayerState::AShooterPlayerState()
{
	TeamNumber = 0;
	NumKills = 0;
	NumDeaths = 0;
	bQuitter = false;
	KillsSinceLastDeath = 0;
	LastKillTime = -MAX_FLT;
	MultiKills = 1;
	LivesRemaining = -1;

	for (int32 i=0; i < GNumPlayerColors; i++)
	{
		PlayerColors.Add(FLinearColor::White);
	}
}

void AShooterPlayerState::Reset()
{
	Super::Reset();
	
	//PlayerStates persist across seamless travel.  Keep the same teams as previous match.
	//SetTeamNum(0);
	NumKills = 0;
	NumDeaths = 0;
	KillsSinceLastDeath = 0;
	LastKillTime = -MAX_FLT;
	MultiKills = 1;
}

void AShooterPlayerState::UnregisterPlayerWithSession()
{
	if (!IsFromPreviousLevel())
	{
		Super::UnregisterPlayerWithSession();
	}
}

void AShooterPlayerState::ClientInitialize(class AController* InController)
{
	Super::ClientInitialize(InController);
	
	AShooterPlayerController* PC = Cast<AShooterPlayerController>(InController);
	if (PC && PC->IsLocalController())
	{
		// notify server of my colors for broadcasting, if not a team game
		UShooterLocalPlayer* ShooterLP = Cast<UShooterLocalPlayer>(PC->GetNetOwningPlayer());
		UShooterPersistentUser* PersistentUser = ShooterLP ? ShooterLP->GetPersistentUser() : NULL;
		if (PersistentUser)
		{
			for (int32 i = 0; i < PersistentUser->GetNumColors(); i++)
			{
				ServerSetColor(i, PersistentUser->GetColor(i));
			}
			//PC->SetName(PersistentUser->GetPlayerName());
			//SetPlayerName(PersistentUser->GetPlayerName());
		}
	}
}

bool AShooterPlayerState::ServerSetTeamNum_Validate(uint8 NewTeamNumber)
{
	return true;
}
// server
void AShooterPlayerState::ServerSetTeamNum_Implementation(uint8 NewTeamNumber)
{
	AShooterGameMode* Game = GetWorld()->GetAuthGameMode<AShooterGameMode>();
	if (Game && NewTeamNumber < Game->GameModeInfo.MaxTeams)
	{
		TeamNumber = NewTeamNumber;
		AShooterGameState* const MyGameState = GetWorld()->GetGameState<AShooterGameState>();
		if (MyGameState)
		{
			//server -- set player colors (but keep the Roughness they choose)
			if (MyGameState->bChangeToTeamColors)
			{
				for (int32 i = 0; i < GetNumColors(); i++)
				{
					const float MyRoughness = PlayerColors[i].A;
					UShooterGameInstance* GI = GetWorld()->GetGameInstance<UShooterGameInstance>();
					if (GI)
					{
						FLinearColor TeamColor = GI->GetTeamColor(TeamNumber);
						TeamColor.A = MyRoughness;
						ServerSetColor_Implementation(i, TeamColor);
					}
				}
			}
		}
		AShooterGameMode_TeamDeathMatch* GMTDM = Cast<AShooterGameMode_TeamDeathMatch>(GetWorld()->GetAuthGameMode());
		AShooterAIController* BotController = Cast<AShooterAIController>(GetOwner());
		if (GMTDM && !BotController)
		{
			GMTDM->PlayerChangedToTeam(this, NewTeamNumber);
		}
		AController* OwnerController = Cast<AController>(GetOwner());
		AShooterCharacter* ShooterCharacter = OwnerController ? Cast<AShooterCharacter>(OwnerController->GetCharacter()) : NULL;
		if (ShooterCharacter != NULL)
		{
			ShooterCharacter->Suicide();
		}
	}
}

void AShooterPlayerState::SetQuitter(bool bInQuitter)
{
	bQuitter = bInQuitter;
}

void AShooterPlayerState::CopyProperties(class APlayerState* PlayerState)
{	
	Super::CopyProperties(PlayerState);

	AShooterPlayerState* ShooterPlayer = Cast<AShooterPlayerState>(PlayerState);
	if (ShooterPlayer)
	{
		ShooterPlayer->TeamNumber = TeamNumber;
	}	
}

bool AShooterPlayerState::ServerSetColor_Validate(uint8 ColorIndex, FLinearColor NewColor)
{
	return ColorIndex < GetNumColors();
}

// server
void AShooterPlayerState::ServerSetColor_Implementation(uint8 ColorIndex, FLinearColor NewColor)
{
	
	FLinearColor TheColor = NewColor;
	AShooterGameState* const MyGameState = GetWorld()->GetGameState<AShooterGameState>();
	if (MyGameState && MyGameState->bChangeToTeamColors)
	{
		UShooterGameInstance* GI = GetWorld()->GetGameInstance<UShooterGameInstance>();
		if (GI)
		{
			TheColor = GI->GetTeamColor(GetTeamNum());
		}
	}
	PlayerColors[ColorIndex] = NewColor;
	AController* OwnerController = Cast<AController>(GetOwner());
	AShooterCharacter* ShooterCharacter = OwnerController ? Cast<AShooterCharacter>(OwnerController->GetCharacter()) : NULL;
	if (ShooterCharacter != NULL)
	{
		BroadcastNewColor(ColorIndex, TheColor, ShooterCharacter);
	}
}

// server -> server+clients
void AShooterPlayerState::BroadcastNewColor_Implementation(uint8 ColorIndex, FLinearColor NewColor, AShooterCharacter* OwnerCharacter)
{
	PlayerColors[ColorIndex] = NewColor;
	OwnerCharacter->UpdatePlayerColorsAllMIDs();
}

void AShooterPlayerState::SetColorLocal(uint8 ColorIndex, FLinearColor NewColor)
{
	check(ColorIndex < GetNumColors());
	PlayerColors[ColorIndex] = NewColor;
	AController* OwnerController = Cast<AController>(GetOwner());
	if (OwnerController != NULL)
	{
		AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OwnerController->GetCharacter());
		if (ShooterCharacter != NULL)
		{
			ShooterCharacter->UpdatePlayerColorsAllMIDs();
		}
	}
}


void AShooterPlayerState::UpdateAllColors()
{
	for (int32 i = 0; i < PlayerColors.Num(); i++)
	{
		SetColorLocal(i, PlayerColors[i]);
	}
}

FLinearColor AShooterPlayerState::GetColor(uint8 ColorIndex) const
{
	check(ColorIndex < PlayerColors.Num());
	return PlayerColors[ColorIndex];
}

uint8 AShooterPlayerState::GetTeamNum() const
{
	return TeamNumber;
}

int32 AShooterPlayerState::GetKills() const
{
	return NumKills;
}

int32 AShooterPlayerState::GetDeaths() const
{
	return NumDeaths;
}

int32 AShooterPlayerState::GetSuicides() const
{
	return NumSuicides;
}

bool AShooterPlayerState::IsQuitter() const
{
	return bQuitter;
}

void AShooterPlayerState::ScoreKill(AShooterPlayerState* Victim, int32 Points)
{
	NumKills++;
	ScorePoints(Points);
	KillsSinceLastDeath++;
	AShooterGameState* ShooterGameState = GetWorld()->GetGameState<AShooterGameState>();
	UShooterMessageHandler* MessageHandlerInst = ShooterGameState ? ShooterGameState->GetMessageHandler() : NULL;
	if (MessageHandlerInst)
	{
		if (GetWorld()->GetTimeSeconds() - LastKillTime <= MessageHandlerInst->MultiKillMaxTime)
		{
			MultiKills++;
		}
		else
		{
			MultiKills = 1;
		}
	}
	LastKillTime = GetWorld()->GetTimeSeconds();
}

void AShooterPlayerState::ScoreDeath(AShooterPlayerState* KilledBy, int32 Points)
{
	NumDeaths++;
	ScorePoints(Points);
}

void AShooterPlayerState::ScoreSuicide(int32 Points)
{
	NumSuicides++;
	ScorePoints(Points);
}

void AShooterPlayerState::ScorePoints(int32 Points)
{
	AShooterGameState* const MyGameState = Cast<AShooterGameState>(GetWorld()->GetGameState());
	if (MyGameState && MyGameState->GetNumTeams() > 0 && MyGameState->bPlayersAddTeamScore)
	{
		MyGameState->AddTeamScore(TeamNumber, Points);
	}
	SetScore(GetScore() + Points);
}

void AShooterPlayerState::InformAboutKill_Implementation(class AShooterPlayerState* KillerPlayerState, class AShooterPlayerState* KilledPlayerState)
{
	//id can be null for bots
	if (KillerPlayerState->GetUniqueId().IsValid())
	{	
		//search for the actual killer before calling OnKill()	
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{		
			AShooterPlayerController* TestPC = Cast<AShooterPlayerController>(It->Get());
			if (TestPC && TestPC->IsLocalController())
			{
				// a local player might not have an ID if it was created with CreateDebugPlayer.
				ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(TestPC->Player);
				FUniqueNetIdRepl LocalID = LocalPlayer->GetCachedUniqueNetId();
				if (LocalID.IsValid() &&  *LocalPlayer->GetCachedUniqueNetId() == *KillerPlayerState->GetUniqueId())
				{			
					TestPC->OnKill();
				}
			}
		}
	}
}

void AShooterPlayerState::BroadcastDeath_Implementation(class AShooterPlayerState* KillerPlayerState, TSubclassOf<class AShooterWeapon> KillerWeaponClass, TSubclassOf<class UShooterDamageType> KillerDmgType)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		// all local players get death messages so they can update their huds.
		AShooterPlayerController* TestPC = Cast<AShooterPlayerController>(It->Get());
		if (TestPC && TestPC->IsLocalController())
		{
			TestPC->OnDeathMessage(KillerPlayerState, this, KillerWeaponClass, KillerDmgType);
		}
	}	
}

void AShooterPlayerState::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( AShooterPlayerState, TeamNumber );
	DOREPLIFETIME( AShooterPlayerState, PlayerColors );
	DOREPLIFETIME( AShooterPlayerState, NumKills );
	DOREPLIFETIME(AShooterPlayerState, NumDeaths);
	DOREPLIFETIME(AShooterPlayerState, NumSuicides);
	DOREPLIFETIME(AShooterPlayerState, LivesRemaining);
}

void AShooterPlayerState::RestartPlayer()
{
	KillsSinceLastDeath = 0;
}

void AShooterPlayerState::UseLife()
{
	if (LivesRemaining > -1)
	{
		LivesRemaining--;
	}
}

void AShooterPlayerState::OnRep_LivesRemaining()
{
	AShooterPlayerController* OwnerController = Cast<AShooterPlayerController>(GetOwner());
	if (OwnerController != NULL)
	{
		AShooterSpectatorPawn* Spec = Cast<AShooterSpectatorPawn>(OwnerController->GetSpectatorPawn());
		const bool bAllowFreeCamAndFocus = LivesRemaining == 0;
		if (Spec)
		{
			Spec->bAllowFreeCam = bAllowFreeCamAndFocus;
			Spec->bAllowSwitchFocus = bAllowFreeCamAndFocus;
		}
	}
}

int32 AShooterPlayerState::GetLivesRemaining() const
{
	return LivesRemaining;
}


void AShooterPlayerState::OnRep_PlayerColors()
{
	UpdateAllColors();
}