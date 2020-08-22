// Copyright 2013-2014 Rampaging Blue Whale Games. All rights reserved.

#include "ShooterGameInstance.h"
#include "Player/ShooterPlayerController.h"
#include "Player/ShooterLocalPlayer.h"
#include "GameRules/ShooterGameState.h"
#include "GameRules/ShooterGameMode.h"
#include "GameRules/ShooterGameSession.h"

UShooterGameInstance::UShooterGameInstance()
{
	bIsOnline = true;
	FillMapList();
	OnEndSessionCompleteDelegate = FOnEndSessionCompleteDelegate::CreateUObject(this, &UShooterGameInstance::OnEndSessionComplete);

	TeamColors.Add(FLinearColor::Red);
	TeamColors.Add(FLinearColor::Blue);
	TeamColors.Add(FLinearColor::Green);
	TeamColors.Add(FLinearColor::Yellow);
	TeamColors.Add(FLinearColor(0.f, 1.f, 1.f)); //cyan
	TeamColors.Add(FLinearColor(1.f, 0.f, 1.f)); //magenta
	TeamColors.Add(FLinearColor::White);

	TeamNames.Add(NSLOCTEXT("Colors", "Red", "Red Team"));
	TeamNames.Add(NSLOCTEXT("Colors", "Blue", "Blue Team"));
	TeamNames.Add(NSLOCTEXT("Colors", "Green", "Green Team"));
	TeamNames.Add(NSLOCTEXT("Colors", "Yellow", "Yellow Team"));
	TeamNames.Add(NSLOCTEXT("Colors", "Cyan", "Cyan Team"));
	TeamNames.Add(NSLOCTEXT("Colors", "Magenta", "Magenta Team"));
	TeamNames.Add(NSLOCTEXT("Colors", "White", "White Team"));
}


AShooterGameSession* UShooterGameInstance::GetGameSession() const
{
	UWorld* const World = GetWorld();
	if (World)
	{
		AGameModeBase* const Game = World->GetAuthGameMode();
		if (Game)
		{
			return Cast<AShooterGameSession>(Game->GameSession);
		}
	}
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
// Online stuff
///////////////////////////////////////////////////////////////////////////////

// starts playing a game as the host
bool UShooterGameInstance::HostGame(ULocalPlayer* LocalPlayer, const FString& GameModePrefix, const FString& MapFileName, const bool bIsLanMatch, const int32 MaxNumPlayers, const int32 NumBots, const int32 NumLocalPlayers, const int32 ScoreLimit, const int32 RoundTime, const uint8 NumTeams)
{
	//GameInstance->NumDesiredLocalPlayers = NumLocalPlayers;
	//GameInstance->SetIsOnline(true);
	TravelURL = FString::Printf(TEXT("/Game/Maps/%s?game=%s?listen%s?%s=%d?ScoreLimit=%d?RoundTime=%d?NumTeams=%d"), *MapFileName, *GameModePrefix,
		bIsLanMatch ? TEXT("?bIsLanMatch") : TEXT(""),
		*AShooterGameMode::GetBotsCountOptionName(), NumBots,
		ScoreLimit,
		RoundTime,
		NumTeams);
	if (!GetIsOnline())
	{
		// Offline game, just go straight to map
		GetWorld()->ServerTravel(TravelURL);
		return true;
	}

	// Online game
	AShooterGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		// add callback delegate for completion
		const FString& MapPath = FString::Printf(TEXT("/Game/Maps/%s"), *MapFileName);
		OnCreatePresenceSessionCompleteDelegateHandle = GameSession->OnCreatePresenceSessionComplete().AddUObject(this, &UShooterGameInstance::OnCreatePresenceSessionComplete);
		return GameSession->HostSession(LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId(), GameSessionName, GameModePrefix, MapPath, bIsLanMatch, true, MaxNumPlayers);
	}

	return false;
}

void UShooterGameInstance::SetIsOnline(bool bInIsOnline)
{
	bIsOnline = bInIsOnline;
}

AShooterGameSession * UShooterGameInstance::GetGameSession()
{
	if (GetWorld() && GetWorld()->GetAuthGameMode())
	{
		return Cast<AShooterGameSession>(GetWorld()->GetAuthGameMode()->GameSession);
	}
	return nullptr;
}

/** Callback which is intended to be called upon session creation */
void UShooterGameInstance::OnCreatePresenceSessionComplete(FName SessionName, bool bWasSuccessful)
{
	AShooterGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		GameSession->OnCreatePresenceSessionComplete().Remove(OnCreatePresenceSessionCompleteDelegateHandle);

		// Add the splitscreen player if one exists
		if (bWasSuccessful && LocalPlayers.Num() > 1)
		{
			auto Sessions = Online::GetSessionInterface();
			if (Sessions.IsValid() && LocalPlayers[1]->GetPreferredUniqueNetId().IsValid())
			{
				Sessions->RegisterLocalPlayer(*LocalPlayers[1]->GetPreferredUniqueNetId(), GameSessionName,
					FOnRegisterLocalPlayerCompleteDelegate::CreateUObject(this, &UShooterGameInstance::OnRegisterLocalPlayerComplete));
			}
		}
		else
		{
			// We either failed or there is only a single local user
			FinishSessionCreation(bWasSuccessful ? EOnJoinSessionCompleteResult::Success : EOnJoinSessionCompleteResult::UnknownError);
		}
	}
}

void UShooterGameInstance::OnRegisterLocalPlayerComplete(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result)
{
	FinishSessionCreation(Result);
}

void UShooterGameInstance::FinishSessionCreation(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		BeginPlayingState();
		// Travel to the specified match URL
		GetWorld()->ServerTravel(TravelURL);
	}
	else
	{
		//FText ReturnReason = NSLOCTEXT("NetworkErrors", "CreateSessionFailed", "Failed to create session.");
		//FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");
		//ShowMessageThenGoMain(ReturnReason, OKButton, FText::GetEmpty());
		//@TODO
	}
}


void UShooterGameInstance::BeginServerSearch(bool bLANMatch, const FString & InMapFilterName, const FString & InGameModeFilterName)
{
	bLANMatchSearch = bLANMatch;
	MapFilterName = InMapFilterName;
	GameModeFilterName = InGameModeFilterName;
	bSearchingForServers = true;
	ServerList.Empty();

	ULocalPlayer* LP = GEngine->GetGamePlayer(GetWorld(), 0);
	if (LP != nullptr)
	{
		AShooterGameSession* const GameSession = GetGameSession();
		if (GameSession)
		{
			GameSession->OnFindSessionsComplete().RemoveAll(this);
			OnSearchSessionsCompleteDelegateHandle = GameSession->OnFindSessionsComplete().AddUObject(this, &UShooterGameInstance::OnSearchSessionsComplete);

			GameSession->FindSessions(LP->GetPreferredUniqueNetId().GetUniqueNetId(), GameSessionName, bLANMatchSearch, true);
		}
	}
}

/** Callback which is intended to be called upon finding sessions */
void UShooterGameInstance::OnSearchSessionsComplete(bool bWasSuccessful)
{
	AShooterGameSession* const Session = GetGameSession();
	if (Session)
	{
		int32 CurrentSearchIdx, NumSearchResults;
		EOnlineAsyncTaskState::Type SearchState = Session->GetSearchResultStatus(CurrentSearchIdx, NumSearchResults);
		Session->OnFindSessionsComplete().Remove(OnSearchSessionsCompleteDelegateHandle);
		ServerList.Empty();
		const TArray<FOnlineSessionSearchResult> & SearchResults = Session->GetSearchResults();
		check(SearchResults.Num() == NumSearchResults);

		for (int32 IdxResult = 0; IdxResult < NumSearchResults; ++IdxResult)
		{
			FServerDetails NewServerEntry;

			const FOnlineSessionSearchResult & Result = SearchResults[IdxResult];

			NewServerEntry.ServerName = Result.Session.OwningUserName;
			NewServerEntry.Ping = FString::FromInt(Result.PingInMs);
			NewServerEntry.CurrentPlayers = FString::FromInt(Result.Session.SessionSettings.NumPublicConnections
				+ Result.Session.SessionSettings.NumPrivateConnections
				- Result.Session.NumOpenPublicConnections
				- Result.Session.NumOpenPrivateConnections);
			NewServerEntry.MaxPlayers = FString::FromInt(Result.Session.SessionSettings.NumPublicConnections
				+ Result.Session.SessionSettings.NumPrivateConnections);
			NewServerEntry.SearchResultsIndex = IdxResult;

			Result.Session.SessionSettings.Get(SETTING_GAMEMODE, NewServerEntry.GameType);
			Result.Session.SessionSettings.Get(SETTING_MAPNAME, NewServerEntry.MapName);

			ServerList.Add(NewServerEntry);
		}
		bSearchingForServers = false;

		UpdateServerList();

		OnServerSearchFinished();
	}
}

void UShooterGameInstance::UpdateServerList()
{
	for (int32 i = 0; i < ServerList.Num(); ++i)
	{
		/** Only filter maps if a specific map is specified */
		if ((!MapFilterName.IsEmpty() && MapFilterName.Find(ServerList[i].MapName) == INDEX_NONE) ||
			(!GameModeFilterName.IsEmpty() && ServerList[i].GameType != GameModeFilterName))
		{
			ServerList.RemoveAt(i);
		}
	}
}

void UShooterGameInstance::StopServerSearch()
{
	OnSearchSessionsComplete(true);
}

void UShooterGameInstance::FillMapList()
{
	class FFindMapsVisitor : public IPlatformFile::FDirectoryVisitor
	{
	public:
		FFindMapsVisitor() {}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
		{
			if (!bIsDirectory)
			{
				FString FullFilePath(FilenameOrDirectory);
				if (FPaths::GetExtension(FullFilePath) == TEXT("umap"))
				{
					const FString CleanFilename = FPaths::GetBaseFilename(FullFilePath);
					//only include maps without underscore (e.g. DM-MyMap_Gameplay.umap)
					if (CleanFilename.Find(TEXT("_")) == INDEX_NONE)
					{
						FMapInfo ThisMap;
						ThisMap.ReadMapData(FullFilePath);
						if (ThisMap.bSupportsArenaGameModes)
						{
							MapsFound.Add(ThisMap);
						}
					}
				}
			}
			return true;
		}
		TArray<FMapInfo> MapsFound;
	};

	const FString MapsFolder = FPaths::ProjectContentDir() + TEXT("Maps");
	FFindMapsVisitor Visitor;
	FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*MapsFolder, Visitor);
	MapList = Visitor.MapsFound;
}


bool UShooterGameInstance::JoinSession(ULocalPlayer* LocalPlayer, int32 SessionIndexInSearchResults)
{
	// needs to tear anything down based on current state?

	AShooterGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		//AddNetworkFailureHandlers();

		OnJoinSessionCompleteDelegateHandle = GameSession->OnJoinSessionComplete().AddUObject(this, &UShooterGameInstance::OnJoinSessionComplete);
		if (GameSession->JoinSession(LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId(), GameSessionName, SessionIndexInSearchResults))
		{
			//ShowLoadingScreen();
			//GotoState(ShooterGameInstanceState::Playing);
			return true;
		}
	}
	return false;
}

bool UShooterGameInstance::JoinSession(ULocalPlayer* LocalPlayer, const FOnlineSessionSearchResult& SearchResult)
{
	// needs to tear anything down based on current state?
	AShooterGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		OnJoinSessionCompleteDelegateHandle = GameSession->OnJoinSessionComplete().AddUObject(this, &UShooterGameInstance::OnJoinSessionComplete);
		if (GameSession->JoinSession(LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId(), GameSessionName, SearchResult))
		{
			return true;
		}
	}
	return false;
}

/** Callback which is intended to be called upon finding sessions */
void UShooterGameInstance::OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result)
{
	// unhook the delegate
	AShooterGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		GameSession->OnJoinSessionComplete().Remove(OnJoinSessionCompleteDelegateHandle);
	}

	// Add the splitscreen player if one exists
	if (Result == EOnJoinSessionCompleteResult::Success && LocalPlayers.Num() > 1)
	{
		auto Sessions = Online::GetSessionInterface();
		if (Sessions.IsValid() && LocalPlayers[1]->GetPreferredUniqueNetId().IsValid())
		{
			Sessions->RegisterLocalPlayer(*LocalPlayers[1]->GetPreferredUniqueNetId(), GameSessionName,
				FOnRegisterLocalPlayerCompleteDelegate::CreateUObject(this, &UShooterGameInstance::OnRegisterJoiningLocalPlayerComplete));
		}
	}
	else
	{
		// We either failed or there is only a single local user
		FinishJoinSession(Result);
	}
}

void UShooterGameInstance::FinishJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		return;
	}
	InternalTravelToSession(GameSessionName);
}

void UShooterGameInstance::OnRegisterJoiningLocalPlayerComplete(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result)
{
	FinishJoinSession(Result);
}

void UShooterGameInstance::InternalTravelToSession(const FName& SessionName)
{
	APlayerController * const PlayerController = GetFirstLocalPlayerController();

	if (PlayerController == nullptr)
	{
		return;
	}

	// travel to session
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub == nullptr)
	{
		return;
	}

	FString URL;
	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

	if (!Sessions.IsValid() || !Sessions->GetResolvedConnectString(SessionName, URL))
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("Failed to travel to session upon joining it"));
		return;
	}

	BeginPlayingState();
	PlayerController->ClientTravel(URL, TRAVEL_Absolute);
}

void UShooterGameInstance::CleanupSession()
{
	bool bPendingOnlineOp = false;

	// end online game and then destroy it
	IOnlineSubsystem * OnlineSub = IOnlineSubsystem::Get();
	IOnlineSessionPtr Sessions = (OnlineSub != NULL) ? OnlineSub->GetSessionInterface() : NULL;

	if (Sessions.IsValid())
	{
		EOnlineSessionState::Type SessionState = Sessions->GetSessionState(GameSessionName);
		UE_LOG(LogOnline, Log, TEXT("Session %s is '%s'"), *FName(GameSessionName).ToString(), EOnlineSessionState::ToString(SessionState));

		if (EOnlineSessionState::InProgress == SessionState)
		{
			UE_LOG(LogOnline, Log, TEXT("Ending session %s"), *FName(GameSessionName).ToString());
			OnEndSessionCompleteDelegateHandle = Sessions->AddOnEndSessionCompleteDelegate_Handle(OnEndSessionCompleteDelegate);
			Sessions->EndSession(GameSessionName);
			bPendingOnlineOp = true;
		}
		else if (EOnlineSessionState::Ending == SessionState)
		{
			UE_LOG(LogOnline, Log, TEXT("Waiting for session %s to end on return to main menu"), *FName(GameSessionName).ToString());
			OnEndSessionCompleteDelegateHandle = Sessions->AddOnEndSessionCompleteDelegate_Handle(OnEndSessionCompleteDelegate);
			bPendingOnlineOp = true;
		}
		else if (EOnlineSessionState::Ended == SessionState || EOnlineSessionState::Pending == SessionState)
		{
			UE_LOG(LogOnline, Log, TEXT("Destroying session %s"), *FName(GameSessionName).ToString());
			OnEndSessionCompleteDelegateHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(OnEndSessionCompleteDelegate);
			Sessions->DestroySession(GameSessionName);
			bPendingOnlineOp = true;
		}
		else if (EOnlineSessionState::Starting == SessionState)
		{
			UE_LOG(LogOnline, Log, TEXT("Waiting for session %s to start, and then we will end it to return to main menu"), *FName(GameSessionName).ToString());
			OnEndSessionCompleteDelegateHandle = Sessions->AddOnStartSessionCompleteDelegate_Handle(OnEndSessionCompleteDelegate);
			bPendingOnlineOp = true;
		}
	}

	if (!bPendingOnlineOp)
	{
		GEngine->HandleDisconnect( GetWorld(), GetWorld()->GetNetDriver() );
	}
}

void UShooterGameInstance::OnEndSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Log, TEXT("UShooterGameInstance::OnEndSessionComplete: Session=%s bWasSuccessful=%s"), *FName(SessionName).ToString(), bWasSuccessful ? TEXT("true") : TEXT("false"));

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnEndSessionCompleteDelegate_Handle(OnEndSessionCompleteDelegateHandle);
		}
	}

	// continue
	CleanupSession();
}

void UShooterGameInstance::Shutdown()
{
	EndPlayingState();
	Super::Shutdown();
}

void UShooterGameInstance::EndPlayingState()
{
	// Disallow splitscreen
	if (GetGameViewportClient() != nullptr)
	{
		GetGameViewportClient()->SetForceDisableSplitscreen(true);
	}

	// Clear the players' presence information
	SetPresenceForLocalPlayers(FVariantData(FString(TEXT("OnMenu"))));

	UWorld* const World = GetWorld();
	AShooterGameState* const GameState = World != NULL ? World->GetGameState<AShooterGameState>() : NULL;

	if (GameState)
	{
		// Send round end events for local players
		for (int i = 0; i < LocalPlayers.Num(); ++i)
		{
			auto ShooterPC = Cast<AShooterPlayerController>(LocalPlayers[i]->PlayerController);
			if (ShooterPC)
			{
				// Assuming you can't win if you quit early
				ShooterPC->ClientSendRoundEndEvent(false, GameState->ElapsedTime);
			}
		}

		// Give the game state a chance to cleanup first
		GameState->RequestFinishAndExitToMainMenu();
	}
	CleanupSession();
}

void UShooterGameInstance::SetPresenceForLocalPlayers(const FVariantData& PresenceData)
{
	const auto Presence = Online::GetPresenceInterface();
	if (Presence.IsValid())
	{
		for (int i = 0; i < LocalPlayers.Num(); ++i)
		{
			FUniqueNetIdRepl UserId = LocalPlayers[i]->GetPreferredUniqueNetId();

			if (UserId.IsValid())
			{
				FOnlineUserPresenceStatus PresenceStatus;
				PresenceStatus.Properties.Add(DefaultPresenceKey, PresenceData);

				Presence->SetPresence(*UserId, PresenceStatus);
			}
		}
	}
}

void UShooterGameInstance::BeginPlayingState()
{
	//bPendingEnableSplitscreen = true;

	// Set presence for playing in a map
	SetPresenceForLocalPlayers(FVariantData(FString(TEXT("InGame"))));

	// Make sure viewport has focus
	//FSlateApplication::Get().SetAllUserFocusToGameViewport();
}


///////////////////////////////////////////////////////////////////////////////
// Team stuff
///////////////////////////////////////////////////////////////////////////////

FLinearColor UShooterGameInstance::GetTeamColor(uint8 Team) const
{
	//@todo: something for color vision deficiency
	if (Team < GetMaxTeams())
	{
		return TeamColors[Team];
	}
	return FLinearColor::Gray;
}

FText UShooterGameInstance::GetTeamName(uint8 Team) const
{
	if (Team < GetMaxTeams())
	{
		return TeamNames[Team];
	}
	return NSLOCTEXT("General", "Undefined", "Undefined");
}

uint8 UShooterGameInstance::GetMaxTeams() const
{
	return FMath::Min(TeamColors.Num(), TeamNames.Num());
}
