// Copyright 2013-2014 Rampaging Blue Whale Games. All rights reserved.

#pragma once

#include "GameFramework/GameMode.h"
#include "ShooterTypes.h"
#include "ShooterGameMode_Menu.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERGAME_API AShooterGameMode_Menu : public AGameMode
{
	GENERATED_BODY()

public:
	AShooterGameMode_Menu();

	// Begin AGameMode interface
	/** skip it, menu doesn't require player start or pawn */
	virtual void RestartPlayer(class AController* NewPlayer) override;

	/** Returns game session class to use */
	virtual TSubclassOf<AGameSession> GetGameSessionClass() const override;
	// End AGameMode interface

	TArray<struct FGameModeInfo> GetGameModeList() const;

protected:

	/** game mode aliases (CTF, DM, etc.) */
	UPROPERTY(config)
	TArray<FGameModeInfo> GameModeList;
	
	
};
