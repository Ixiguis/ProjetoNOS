// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "ShooterGameUserSettings.h"
#include "SoundDefinitions.h"
#include "Sound/SoundClass.h"
#include "Player/ShooterPlayerController.h"

/** Get the percentage scale for a given quality level. Copied from Scalability.cpp. */
static int32 GetRenderScaleLevelFromQualityLevel(int32 InQualityLevel)
{
	check(InQualityLevel >= 0 && InQualityLevel <=3);
	static const int32 ScalesForQuality[4] = { 50, 71, 87, 100 }; // Single axis scales which give 25/50/75/100% area scales
	return ScalesForQuality[InQualityLevel];
}

UShooterGameUserSettings::UShooterGameUserSettings()
{
	SetToDefaults();
}

void UShooterGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();
	
	for (int32 i = 0; i < EGraphicsOptions::MAX; i++)
	{
		GraphicsQuality[i] = 3;
	}
	bIsLanMatch = false;
	Gamma = 2.2f;

	AudioMasterVolume = 1.0f;
	AudioMusicVolume = 1.0f;
	AudioSFXVolume = 1.0f;
	AudioAnnouncerVolume = 1.0f;
	AudioPickupsVolume = 1.0f;
	AudioUIVolume = 1.0f;
	bVoiceChatEnabled = false;
	VoiceChatVolume = 1.0f;
	AudioAmbientVolume = 1.0f;

	SelectedGameMode = 0;
	bInvertedYAxis = false;
	AimSensitivity = 1.0f;
	BotsCount = 1;
	PlayerSlots = 8;
}

void UShooterGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	for (int32 i = 0; i < EGraphicsOptions::MAX; i++)
	{
		switch (i)
		{
		case EGraphicsOptions::AntiAliasingQuality:
			ScalabilityQuality.AntiAliasingQuality = GraphicsQuality[EGraphicsOptions::AntiAliasingQuality];
			break;
		case EGraphicsOptions::EffectsQuality:
			ScalabilityQuality.EffectsQuality = GraphicsQuality[EGraphicsOptions::EffectsQuality];
			break;
		case EGraphicsOptions::PostProcessQuality:
			ScalabilityQuality.PostProcessQuality = GraphicsQuality[EGraphicsOptions::PostProcessQuality];
			break;
		case EGraphicsOptions::ResolutionQuality:
			ScalabilityQuality.ResolutionQuality = GetRenderScaleLevelFromQualityLevel(GraphicsQuality[EGraphicsOptions::ResolutionQuality]);
			break;
		case EGraphicsOptions::ShadowQuality:
			ScalabilityQuality.ShadowQuality = GraphicsQuality[EGraphicsOptions::ShadowQuality];
			break;
		case EGraphicsOptions::TextureQuality:
			ScalabilityQuality.TextureQuality = GraphicsQuality[EGraphicsOptions::TextureQuality];
			break;
		case EGraphicsOptions::ViewDistanceQuality:
			ScalabilityQuality.ViewDistanceQuality = GraphicsQuality[EGraphicsOptions::ViewDistanceQuality];
			break;
		}
	}

	GEngine->DisplayGamma =  Gamma;

	ApplyAudioSettings();

	Super::ApplySettings(bCheckForCommandLineOverrides);
}

void UShooterGameUserSettings::ApplyAudioSettings()
{
	ChangeSoundClassVolume("Master", AudioMasterVolume);
	ChangeSoundClassVolume("SFX", AudioSFXVolume);
	ChangeSoundClassVolume("Music", AudioMusicVolume);
	ChangeSoundClassVolume("Voice", VoiceChatVolume);
	ChangeSoundClassVolume("Announcer", AudioAnnouncerVolume);
	ChangeSoundClassVolume("Pickup", AudioPickupsVolume);
	ChangeSoundClassVolume("UI", AudioUIVolume);
	ChangeSoundClassVolume("Ambient", AudioAmbientVolume);
}

int32 ShooterGameGetBoundFullScreenModeCVar()
{
	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FullScreenMode")); 

	if (FPlatformProperties::SupportsWindowedMode())
	{
		int32 Value = CVar->GetValueOnGameThread();

		if(Value >= 0 && Value <= 2)
		{
			return Value;
		}
	}

	// every other value behaves like 0
	return 0;
}

// depending on bFullscreen and the console variable r.FullScreenMode
EWindowMode::Type ShooterGameGetWindowModeType(bool bFullscreen)
{
	int32 FullScreenMode = ShooterGameGetBoundFullScreenModeCVar();

	if (FPlatformProperties::SupportsWindowedMode())
	{
		if(!bFullscreen)
		{
			return EWindowMode::Windowed;
		}

		if(FullScreenMode == 2)
		{
			return EWindowMode::WindowedFullscreen;
		}
	}
	return EWindowMode::Fullscreen;
}

EWindowMode::Type UShooterGameUserSettings::GetCurrentFullscreenMode() const
{
	EWindowMode::Type CurrentFullscreenMode = EWindowMode::Windowed;
	if ( GEngine && GEngine->GameViewport && GEngine->GameViewport->ViewportFrame )
	{
		bool bIsCurrentlyFullscreen = GEngine->GameViewport->IsFullScreenViewport();
		CurrentFullscreenMode = ShooterGameGetWindowModeType(bIsCurrentlyFullscreen);
	}
	return CurrentFullscreenMode;
}

void UShooterGameUserSettings::ChangeSoundClassVolume(FString ClassName, float Volume)
{
	FAudioDeviceHandle Device = GEngine->GetMainAudioDevice();
	if (!Device.IsValid())
	{
		return;
	}
	TMap<USoundClass*, FSoundClassProperties> Classes = Device.GetAudioDevice()->GetSoundClassPropertyMap();
	for (auto It = Classes.CreateConstIterator(); It; ++It)
	{
		USoundClass* SoundClass = It.Key();
		if (SoundClass && SoundClass->GetFullName().Find(ClassName, ESearchCase::IgnoreCase) != INDEX_NONE)
		{
			SoundClass->Properties.Volume = Volume;
		}
	}
}

int32 UShooterGameUserSettings::GetGraphicsQuality(TEnumAsByte<EGraphicsOptions::Type> GraphicSetting) const
{
	return GraphicsQuality[GraphicSetting];
}

void UShooterGameUserSettings::SetGraphicsQuality(TEnumAsByte<EGraphicsOptions::Type> GraphicSetting, int32 InGraphicsQuality)
{
	GraphicsQuality[GraphicSetting] = InGraphicsQuality;
}

void UShooterGameUserSettings::SetResolutionAndFullScreen(int32 ResX, int32 ResY, bool bFullScreen)
{
	SetScreenResolution(FIntPoint(ResX, ResY));
	SetFullscreenMode(bFullScreen ? EWindowMode::Fullscreen : EWindowMode::Windowed);
	SaveSettings();
}

bool UShooterGameUserSettings::IsFullScreen() const
{
	return GetFullscreenMode() == EWindowMode::Fullscreen || GetFullscreenMode() == EWindowMode::WindowedFullscreen;
}

FString UShooterGameUserSettings::GetCurrentResolution() const
{
	const FIntPoint CurRes = GetScreenResolution();
	return FString::FromInt(CurRes.X) + TEXT("x") + FString::FromInt(CurRes.Y);
}

void UShooterGameUserSettings::SetVoiceChatEnabled(bool bEnable)
{
	bVoiceChatEnabled = bEnable;
	for (FConstPlayerControllerIterator It = GWorld->GetPlayerControllerIterator(); It; ++It)
	{
		AShooterPlayerController* SPC = Cast<AShooterPlayerController>(It->Get());
		if (SPC)
		{
			SPC->UpdateChatOption();
		}
	}
	SaveSettings();
}

bool UShooterGameUserSettings::IsVoiceChatEnabled()
{
	return bVoiceChatEnabled;
}
