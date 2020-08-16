// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#pragma once

#include "ShooterMessageHandler.generated.h"

UENUM()
namespace EMessageTypes
{
	enum Type
	{
		FirstBlood=0,
		Headshot=1,
		MultiKill=2,
		KillingSpree=3,
		KillingSpreeEnded=4,
		NUM_ACHIEVEMENTS=9,

		FlagTaken=10,
		FlagDropped=11,
		FlagRecovered=12,
		FlagCaptured=13,
		NUM_CTF=19
	};
}

USTRUCT(BlueprintType)
struct FGameMessage
{
	GENERATED_USTRUCT_BODY()
	
	/** Text to display on the local user (the one this message is relative to, e.g. "You are on a killing spree") */
	UPROPERTY(EditDefaultsOnly, Category = Messages)
	FText LocalMessageText;

	/** Sound to play (use the nodes remote/local on the sound cue) */
	UPROPERTY(EditDefaultsOnly, Category = Messages)
	USoundCue* Sound;
	
	/** If not empty, this text will display on remote users (e.g. "{KillerName} is on a killing spree") */
	UPROPERTY(EditDefaultsOnly, Category = Messages)
	FText RemoteMessageText;	
};

/**
 * Class that handles gameplay and announcer messages (e.g. Red Flag Taken, Double Kill, Headshot, etc).
 * Instanced and used by the Game Mode.
 */
UCLASS(Blueprintable, Abstract)
class SHOOTERGAME_API UShooterMessageHandler : public UObject
{
	GENERATED_BODY()

public:
	UShooterMessageHandler();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=CTF)
	FGameMessage FlagTakenMessage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=CTF)
	FGameMessage FlagDroppedMessage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=CTF)
	FGameMessage FlagRecoveredMessage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=CTF)
	FGameMessage FlagCapturedMessage;
	
	/** Headshot message */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=MiscMessages)
	FGameMessage HeadshotMessage;
	
	/** First Blood message */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=MiscMessages)
	FGameMessage FirstBloodMessage;
	
	/** The killing spree messages */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=KillingSpree)
	TArray<FGameMessage> KillingSpreeMessages;
	
	/** Killing spree ended messages (for each killing spree level, should be in the same number as KillingSpreeMessages) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=KillingSpree)
	TArray<FGameMessage> KillingSpreeEndedMessages;
	
	/** how many kills you need to increase your killing spree level */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=KillingSpree)
	uint8 KillsForSpreeIncrease;

	/** The sequential kill messages (double kill, multi kill, etc); the last one will be repeatedly used if someone keeps killing */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=MultiKill)
	TArray<FGameMessage> MultiKillMessages;
	
	/** max amount of time (seconds) that may pass to increase your sequential kill level */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=MultiKill)
	float MultiKillMaxTime;

};
