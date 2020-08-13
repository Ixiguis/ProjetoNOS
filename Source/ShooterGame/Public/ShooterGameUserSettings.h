// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once

#include "ShooterGameUserSettings.generated.h"

UENUM(BlueprintType)
namespace EGraphicsOptions
{
	enum Type
	{
		ResolutionQuality,
		ViewDistanceQuality,
		AntiAliasingQuality,
		ShadowQuality,
		PostProcessQuality,
		TextureQuality,
		EffectsQuality,
		MAX
	};
}

UCLASS()
class UShooterGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	UShooterGameUserSettings();

	/** Applies all settings (video, audio, etc). */
	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;
	
	// interface UGameUserSettings
	virtual void SetToDefaults() override;
	
	/////////////////////////////////////////////////
	// VIDEO SETTINGS

	UPROPERTY(BlueprintReadWrite, Category=Options, config)
	float Gamma;

	UFUNCTION(BlueprintCallable, Category=Options)
	void SetResolutionAndFullScreen(int32 ResX, int32 ResY, bool bFullScreen);

	UFUNCTION(BlueprintCallable, Category=Options)
	bool IsFullScreen() const;

	/** Returns the current resolution, in the format e.g. 1920x1080. */
	UFUNCTION(BlueprintCallable, Category=Options)
	FString GetCurrentResolution() const;
	
	UFUNCTION(BlueprintCallable, Category=Options)
	int32 GetGraphicsQuality(TEnumAsByte<EGraphicsOptions::Type> GraphicSetting) const;

	UFUNCTION(BlueprintCallable, Category=Options)
	void SetGraphicsQuality(TEnumAsByte<EGraphicsOptions::Type> GraphicSetting, int32 InGraphicsQuality);

	/** Gets current fullscreen mode */
	EWindowMode::Type GetCurrentFullscreenMode() const;

	/////////////////////////////////////////////////
	// AUDIO SETTINGS
	
	/** Applies volume settings only. */
	UFUNCTION(BlueprintCallable, Category=Options)
	void ApplyAudioSettings();

	/** Overall audio volume, ranging from 0.0 to 1.0 */
	UPROPERTY(BlueprintReadWrite, Category=Options, config)
	float AudioMasterVolume;

	UPROPERTY(BlueprintReadWrite, Category=Options, config)
	float AudioSFXVolume;

	UPROPERTY(BlueprintReadWrite, Category=Options, config)
	float AudioMusicVolume;
	
	UPROPERTY(BlueprintReadWrite, Category=Options, config)
	float AudioAnnouncerVolume;
	
	UPROPERTY(BlueprintReadWrite, Category=Options, config)
	float AudioPickupsVolume;
	
	UPROPERTY(BlueprintReadWrite, Category=Options, config)
	float AudioUIVolume;
	
	UPROPERTY(BlueprintReadWrite, Category=Options, config)
	float AudioAmbientVolume;

	UFUNCTION(BlueprintCallable, Category=Options)
	void SetVoiceChatEnabled(bool bEnable);

	bool IsVoiceChatEnabled();

	/** volume of other users' voice */
	UPROPERTY(BlueprintReadWrite, Category=Options, config)
	float VoiceChatVolume;
	
	/** Changes the volume of all sound classes that contain ClassName in their pathname. Case sensitive! */
	void ChangeSoundClassVolume(FString ClassName, float Volume);
	
	/////////////////////////////////////////////////
	// GAME SETTINGS

	/** last selected game mode index on the main menu */
	UPROPERTY(BlueprintReadWrite, Category=Options, config)
	uint8 SelectedGameMode;
	
	/** how many bots to join hosted game */
	UPROPERTY(BlueprintReadWrite, Category=Options, config)
	int32 BotsCount;

	/** Holds the mouse sensitivity */
	UPROPERTY(BlueprintReadWrite, Category=Options, config)
	float AimSensitivity;

	/** Is the y axis inverted or not? */
	UPROPERTY(BlueprintReadWrite, Category=Options, config)
	bool bInvertedYAxis;
	
	/** is lan match? */
	UPROPERTY(BlueprintReadWrite, Category=Options, config)
	bool bIsLanMatch;
	
	/** number of player slots on hosted server */
	UPROPERTY(BlueprintReadWrite, Category=Options, config)
	int32 PlayerSlots;
	
	/** true: player 1 uses keyboard+mouse, player 2 uses first gamepad, player 3 uses second gamepad, etc.
	*	false: player1 uses first gamepad, player 2 uses second gamepad, etc. */
	UPROPERTY(BlueprintReadWrite, Category=Options, config)
	bool bFirstPlayerOnKeyboard;
	
protected:
	/**
	 * Graphics Quality for each graphic setting (textures, effects, resolution, etc)
	 *	0 = Low
	 *	1 = Medium
	 *	2 = High
	 *	3 = Epic
	 */
	UPROPERTY(config)
	int32 GraphicsQuality[EGraphicsOptions::MAX];

	/** Enable online voice chat? */
	UPROPERTY(BlueprintReadOnly, Category=Options, config)
	bool bVoiceChatEnabled;

};
