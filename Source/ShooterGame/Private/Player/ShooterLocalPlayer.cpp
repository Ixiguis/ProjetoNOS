// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "ShooterLocalPlayer.h"
#include "OnlineSubsystemUtilsClasses.h"

UShooterLocalPlayer::UShooterLocalPlayer()
{
}

UShooterPersistentUser* UShooterLocalPlayer::GetPersistentUser() const
{
	// if persistent data isn't loaded yet, load it
	if (PersistentUser == nullptr)
	{
		UShooterLocalPlayer* const MutableThis = const_cast<UShooterLocalPlayer*>(this);
		// casting away constness to enable caching implementation behavior
		MutableThis->LoadPersistentUser();
	}
	return PersistentUser;
}

void UShooterLocalPlayer::LoadPersistentUser()
{
	// if we changed controllerid / user, then we need to load the appropriate persistent user.
	if (PersistentUser != nullptr && ( GetControllerId() != PersistentUser->GetUserIndex() || GetNickname() != PersistentUser->GetName() ) )
	{
		PersistentUser->SaveIfDirty();
		PersistentUser = nullptr;
	}

	if (PersistentUser == NULL)
	{
		// Use the platform id here to be resilient in the face of controller swapping and similar situations.
		FPlatformUserId PlatformId = GetControllerId();

		auto Identity = Online::GetIdentityInterface();
		if (Identity.IsValid() && GetPreferredUniqueNetId().IsValid())
		{
			PlatformId = Identity->GetPlatformUserIdFromUniqueNetId(*GetPreferredUniqueNetId());
		}
		FString SaveName = TEXT("SaveSlot1_") + FString::FromInt(PlatformId);
		PersistentUser = UShooterPersistentUser::LoadPersistentUser2( SaveName, PlatformId );
	}
}

void UShooterLocalPlayer::SetControllerId(int32 NewControllerId)
{
	ULocalPlayer::SetControllerId(NewControllerId);

	// if we changed controllerid / user, then we need to load the appropriate persistent user.
	if (PersistentUser != nullptr && ( GetControllerId() != PersistentUser->GetUserIndex() || GetNickname() != PersistentUser->GetName() ) )
	{
		PersistentUser->SaveIfDirty();
		PersistentUser = nullptr;
	}

	if (!PersistentUser)
	{
		LoadPersistentUser();
	}
}

FString UShooterLocalPlayer::GetNickname() const
{
	FString UserNickName = Super::GetNickname();

	if ( UserNickName.Len() > MAX_PLAYER_NAME_LENGTH )
	{
		UserNickName = UserNickName.Left( MAX_PLAYER_NAME_LENGTH ) + "...";
	}

	bool bReplace = (UserNickName.Len() == 0);
	
	if ( bReplace )
	{
		UserNickName = FString::Printf( TEXT( "Player%i" ), GetControllerId() + 1 );
	}	

	return UserNickName;
}
