// Copyright 2013-2014 Rampaging Blue Whale Games. All rights reserved.

#pragma once

#include "Engine/GameInstance.h"
#include "ShooterGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERGAME_API UShooterGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:

	UShooterGameInstance();

	class AShooterGameSession* GetGameSession() const;
	
	/** Returns true if the game is in online mode */
	inline bool GetIsOnline() const { return bIsOnline; }

	/** Sets the online mode of the game */
	void SetIsOnline(bool bInIsOnline);

	UFUNCTION(BlueprintPure, Category="Session")
	class AShooterGameSession* GetGameSession();
	
	/** array of servers found */
	UPROPERTY(BlueprintReadOnly, Category="Session")
	TArray<FServerDetails> ServerList;
	
	/** Starts searching for servers */
	UFUNCTION(BlueprintCallable, Category="Session")
	void BeginServerSearch(bool bLANMatch, const FString & InMapFilterName, const FString & InGameModeFilterName);
	
	UFUNCTION(BlueprintCallable, Category="Session")
	void StopServerSearch();

protected:
	
	/** Starts a game as a host */
	UFUNCTION(BlueprintCallable, Category="Session")
	bool HostGame(ULocalPlayer* LocalPlayer, const FString& GameModePrefix, const FString& MapFileName, const bool bIsLanMatch, const int32 MaxNumPlayers, const int32 NumBots, const int32 NumLocalPlayers, const int32 ScoreLimit, const int32 RoundTime, const uint8 NumTeams);

	UFUNCTION(BlueprintCallable, Category = "Session")
	bool JoinSession(ULocalPlayer* LocalPlayer, int32 SessionIndexInSearchResults);

	bool JoinSession(ULocalPlayer* LocalPlayer, const FOnlineSessionSearchResult& SearchResult);

	/** Whether last searched for LAN (so spacebar works) */
	bool bLANMatchSearch;

	/** Whether we're searching for servers */
	UPROPERTY(BlueprintReadOnly, Category="Session")
	bool bSearchingForServers;

	UPROPERTY(BlueprintReadOnly, Category="Game")
	TArray<FMapInfo> MapList;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Game")
	TArray<FGameModeInfo> GameModeList;
	
	/** Called when server search is finished */
	UFUNCTION(BlueprintImplementableEvent, Category = "Session")
	void OnServerSearchFinished();

	void FillMapList();

	/** fill/update server list, should be called before showing this control */
	void UpdateServerList();

	/** Map filter name to use during server searches */
	FString MapFilterName;

	/** Map filter name to use during server searches */
	FString GameModeFilterName;

	/** URL to travel to after pending network operations */
	FString TravelURL;

	/** Whether the match is online or not */
	bool bIsOnline;

	/** Callback which is intended to be called upon session creation */
	void OnCreatePresenceSessionComplete(FName SessionName, bool bWasSuccessful);
	FDelegateHandle OnCreatePresenceSessionCompleteDelegateHandle;

	/** Callback which is intended to be called upon finding sessions */
	void OnSearchSessionsComplete(bool bWasSuccessful);
	FDelegateHandle OnSearchSessionsCompleteDelegateHandle;

	/** Called after all the local players are registered */
	void FinishSessionCreation(EOnJoinSessionCompleteResult::Type Result);

	/** Callback which is called after adding local users to a session */
	void OnRegisterLocalPlayerComplete(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result);

	/** Callback which is intended to be called upon joining session */
	void OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result);
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;

	/** Called after all the local players are registered in a session we're joining */
	void FinishJoinSession(EOnJoinSessionCompleteResult::Type Result);

	/** Callback which is called after adding local users to a session we're joining */
	void OnRegisterJoiningLocalPlayerComplete(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result);

	/** Travel directly to the named session */
	void InternalTravelToSession(const FName& SessionName);

	virtual void Shutdown() override;

	/** destroy active sessions */
	void CleanupSession();

	/** Delegate for ending a session */
	FOnEndSessionCompleteDelegate OnEndSessionCompleteDelegate;
	FDelegateHandle OnEndSessionCompleteDelegateHandle;
	void OnEndSessionComplete(FName SessionName, bool bWasSuccessful);

	/** call when a game is finished / the server is closed */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void EndPlayingState();

	/** called when a game starts */
	void BeginPlayingState();

	void SetPresenceForLocalPlayers(const FVariantData& PresenceData);
};
