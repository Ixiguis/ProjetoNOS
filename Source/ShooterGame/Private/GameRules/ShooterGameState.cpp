// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "GameRules/ShooterGameState.h"
#include "GameRules/ShooterGameMode.h"
#include "UI/ShooterMessageHandler.h"
#include "Player/ShooterPlayerController.h"
#include "Player/ShooterPlayerState.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogShooterGameState);

AShooterGameState::AShooterGameState()
{
	NumTeams = 0;
	RemainingTime = 0;
	bTimerPaused = false;
	bChangeToTeamColors = false;
	bPlayersAddTeamScore = true;
	//bReplicates = true;
	//bAlwaysRelevant = true;
	//SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	
	static ConstructorHelpers::FClassFinder<UShooterMessageHandler> MsgHandlerOb(TEXT("/Game/UI/MessageHandler.MessageHandler_C"));
	MessageHandlerClass = MsgHandlerOb.Class;
}

void AShooterGameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	MessageHandlerInst = NewObject<UShooterMessageHandler>(GetTransientPackage(), MessageHandlerClass, TEXT("MsgHandler"), RF_ClassDefaultObject | RF_Transient | RF_Public | RF_MarkAsNative);

	// call GameModeAndStateInitialized on the server player too
	if (GetLocalRole() == ROLE_Authority)
	{
		AShooterPlayerController* ServerPC = GetWorld()->GetFirstPlayerController<AShooterPlayerController>();
		if (ServerPC && ServerPC->GetNetMode() == NM_ListenServer)
		{
			ServerPC->GameModeAndStateInitialized();
		}
	}
}

void AShooterGameState::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME(AShooterGameState, NumTeams);
	DOREPLIFETIME(AShooterGameState, RemainingTime);
	DOREPLIFETIME(AShooterGameState, bTimerPaused);
	DOREPLIFETIME(AShooterGameState, TeamScores);
	
	DOREPLIFETIME_CONDITION(AShooterGameState, bClientSideHitVerification, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AShooterGameState, bReplicateProjectiles, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AShooterGameState, bChangeToTeamColors, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AShooterGameState, bPlayersAddTeamScore, COND_InitialOnly);
}

void AShooterGameState::GetRankedMap(int32 TeamIndex, RankedPlayerMap& OutRankedMap) const
{
	OutRankedMap.Empty();

	//first, we need to go over all the PlayerStates, grab their score, and rank them
	TMultiMap<int32, AShooterPlayerState*> SortedMap;
	for(int32 i = 0; i < PlayerArray.Num(); ++i)
	{
		int32 Score = 0;
		AShooterPlayerState* CurPlayerState = Cast<AShooterPlayerState>(PlayerArray[i]);
		if (CurPlayerState && (CurPlayerState->GetTeamNum() == TeamIndex))
		{
			SortedMap.Add(FMath::TruncToInt(CurPlayerState->GetScore()), CurPlayerState);
		}
	}

	//sort by the keys
	SortedMap.KeySort(TGreater<int32>());

	//now, add them back to the ranked map
	OutRankedMap.Empty();

	int32 Rank = 0;
	for(TMultiMap<int32, AShooterPlayerState*>::TIterator It(SortedMap); It; ++It)
	{
		OutRankedMap.Add(Rank++, It.Value());
	}
	
}

void AShooterGameState::AddTeamScore(uint8 TeamNumber, int32 ScoreToAdd)
{
	if (TeamNumber >= 0)
	{
		TeamScores[TeamNumber] += ScoreToAdd;
	}
}

void AShooterGameState::SetTeamScore(uint8 TeamNumber, int32 NewScoe)
{
	if (TeamNumber >= 0)
	{
		TeamScores[TeamNumber] = NewScoe;
	}
}

void AShooterGameState::InitTeamScores()
{
	TeamScores.Empty();
	TeamScores.AddZeroed(NumTeams);
}

int32 AShooterGameState::GetTotalKills()
{
	int32 Total = 0;
	for (APlayerState* PS : PlayerArray)
	{
		AShooterPlayerState* SPS = Cast<AShooterPlayerState>(PS);
		if (SPS)
		{
			Total += SPS->GetKills();
		}
	}
	return Total;
}

bool AShooterGameState::IsTeamGame() const
{
	return NumTeams > 1;
}

int32 AShooterGameState::GetPlayerPosition(AShooterPlayerState * Player) const
{
	if (!Player)
	{
		return 0;
	}
	RankedPlayerMap PlayerStateMap;
	const uint8 MyTeam = Player->GetTeamNum();
	GetRankedMap(MyTeam, PlayerStateMap);
	int32 MyPos = *PlayerStateMap.FindKey(Player) + 1;
	return MyPos;
}

int32 AShooterGameState::GetPlayersTeamPosition(AShooterPlayerState * Player) const
{
	if (!Player)
	{
		return 0;
	}
	const uint8 MyTeam = Player->GetTeamNum();
	uint8 MyPos = FMath::Max<uint8>(1, GetNumTeams());
	for (uint8 i = 0; i < GetNumTeams(); i++)
	{
		if (GetNumTeams() > MyTeam &&
			GetTeamScore(MyTeam) >= GetTeamScore(i) && MyTeam != i)
		{
			MyPos--;
		}
	}
	return MyPos;
}

int32 AShooterGameState::GetNumberOfPlayers(int32 Team) const
{
	if (Team == -1)
	{
		return PlayerArray.Num();
	}
	RankedPlayerMap PlayerStateMap;
	GetRankedMap(Team, PlayerStateMap);
	return PlayerStateMap.Num();
}

class UShooterMessageHandler* AShooterGameState::GetMessageHandler() const
{
	return MessageHandlerInst;
}

FGameMessage AShooterGameState::GetGameMessage(TEnumAsByte<EMessageTypes::Type> MessageType, uint8 OptionalRank) const
{
	check(MessageHandlerInst->IsValidLowLevel());
	FGameMessage TheMessage;
	uint8 Rank;
	switch (MessageType)
	{
	case EMessageTypes::FirstBlood:
		TheMessage = MessageHandlerInst->FirstBloodMessage;
		break;
	case EMessageTypes::Headshot:
		TheMessage = MessageHandlerInst->HeadshotMessage;
		break;
	case EMessageTypes::KillingSpree:
		check(MessageHandlerInst->KillingSpreeMessages.Num());
		Rank = FMath::Min<uint8>(OptionalRank, MessageHandlerInst->KillingSpreeMessages.Num()-1);
		TheMessage = MessageHandlerInst->KillingSpreeMessages[Rank];
		break;
	case EMessageTypes::KillingSpreeEnded:
		check(MessageHandlerInst->KillingSpreeEndedMessages.Num());
		Rank = FMath::Min<uint8>(OptionalRank, MessageHandlerInst->KillingSpreeEndedMessages.Num()-1);
		TheMessage = MessageHandlerInst->KillingSpreeEndedMessages[Rank];
		break;
	case EMessageTypes::MultiKill:
		check(MessageHandlerInst->MultiKillMessages.Num());
		Rank = FMath::Min<uint8>(OptionalRank, MessageHandlerInst->MultiKillMessages.Num()-1);
		TheMessage = MessageHandlerInst->MultiKillMessages[Rank];
		break;
	case EMessageTypes::FlagTaken:
		TheMessage = MessageHandlerInst->FlagTakenMessage;
		break;
	case EMessageTypes::FlagDropped:
		TheMessage = MessageHandlerInst->FlagDroppedMessage;
		break;
	case EMessageTypes::FlagRecovered:
		TheMessage = MessageHandlerInst->FlagRecoveredMessage;
		break;
	case EMessageTypes::FlagCaptured:
		TheMessage = MessageHandlerInst->FlagCapturedMessage;
		break;
	default:
		UE_LOG(LogShooterGameMode, Warning, TEXT("Invalid message type: %d"), (uint8)MessageType);
	}
	return TheMessage;
}

void AShooterGameState::RequestFinishAndExitToMainMenu()
{
	if (AuthorityGameMode)
	{
		// we are server, tell the gamemode
		AShooterGameMode* const GameMode = Cast<AShooterGameMode>(AuthorityGameMode);
		if (GameMode)
		{
			GameMode->RequestFinishAndExitToMainMenu();
		}
	}
	else
	{
		// we are client, handle our own business
		//@GI RemoveSplitScreenPlayers

		AShooterPlayerController* const PrimaryPC = Cast<AShooterPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
		if (PrimaryPC)
		{
			check(PrimaryPC->GetNetMode() == ENetMode::NM_Client);
			PrimaryPC->HandleReturnToMainMenu();
		}
	}
}

int32 AShooterGameState::GetTeamScore(uint8 TeamNum) const
{
	return TeamScores[TeamNum];
}

uint8 AShooterGameState::GetNumTeams() const
{
	return NumTeams;
}

void AShooterGameState::SetNumTeams(uint8 n)
{
	NumTeams = n;
	InitTeamScores();
}
