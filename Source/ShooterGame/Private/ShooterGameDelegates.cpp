// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ShooterGameDelegates.h"
#include "GameDelegates.h"
#include "GameRules/ShooterGameState.h"
#include "ShooterEngine.h"
#include "Player/ShooterLocalPlayer.h"

// respond to requests from a companion app
static void WebServerDelegate(int32 UserIndex, const FString& Action, const FString& URL, const TMap<FString, FString>& Params, TMap<FString, FString>& Response)
{
	if (URL == TEXT("/index.html?scoreboard"))
	{
		FString ScoreboardStr = TEXT("{ \"scoreboard\" : [ ");

		// you shouldn't normally use this method to get a UWorld as it won't always be correct in a PIE context.
		// However, the PS4 companion app server will never run in the Editor.
		UGameEngine* GameEngine = CastChecked<UGameEngine>(GEngine);
		if (GameEngine)
		{
			UWorld* World = GameEngine->GetGameWorld();
			if (World)
			{
				ULocalPlayer *Player = GEngine->GetFirstGamePlayer(World);
				if (Player)
				{
					// get the shoter game
					AShooterGameState* const GameState = Cast<AShooterGameState>(Player->PlayerController->GetWorld()->GetGameState());


					RankedPlayerMap Players;
					GameState->GetRankedMap(0, Players);

					bool bNeedsComma = false;
					for (auto It = Players.CreateIterator(); It; ++It)
					{
						if (bNeedsComma)
						{
							ScoreboardStr += TEXT(" ,");
						}
						ScoreboardStr += FString::Printf(TEXT(" { \"n\" : \"%s\" , \"k\" : \"%d\" , \"d\" : \"%d\" }"), *It.Value()->GetShortPlayerName(), It.Value()->GetKills(), It.Value()->GetDeaths());
						bNeedsComma = true;
					}
				}

				ScoreboardStr += TEXT(" ] }");

				Response.Add(TEXT("Content-Type"), TEXT("text/html; charset=utf-8"));
				Response.Add(TEXT("Body"), ScoreboardStr);
			}
		}
	}
}

static void ExtendedSaveGameInfoDelegate(const TCHAR* SaveName, const EGameDelegates_SaveGame Key, FString& Value)
{
	static const int32 MAX_SAVEGAME_SIZE = 100 * 1024;
	switch (Key)
	{
	case EGameDelegates_SaveGame::MaxSize:
		Value = FString::Printf(TEXT("%i"), MAX_SAVEGAME_SIZE);
		break;
	case EGameDelegates_SaveGame::Title:
		Value = TEXT("ShooterGame");
		break;
	case EGameDelegates_SaveGame::SubTitle:
		Value = TEXT("The Shootening");
		break;
	case EGameDelegates_SaveGame::Detail:
		Value = TEXT("ShooterGame User Settings");
		break;
	default:
		break;
	}
}

void InitializeShooterGameDelegates()
{
	FGameDelegates::Get().GetWebServerActionDelegate() = FWebServerActionDelegate::CreateStatic(WebServerDelegate);
	FGameDelegates::Get().GetExtendedSaveGameInfoDelegate() = FExtendedSaveGameInfoDelegate::CreateStatic(ExtendedSaveGameInfoDelegate);
}
