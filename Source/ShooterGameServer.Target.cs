// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

[SupportedPlatforms(UnrealPlatformClass.Server)]
public class ShooterGameServerTarget : TargetRules
{
	public ShooterGameServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		bUsesSteam = true;
		DefaultBuildSettings = BuildSettingsVersion.V2;

		DisablePlugins.Add("Substance");
		ExtraModuleNames.Add("ShooterGame");
		
        GlobalDefinitions.Add("UE4_PROJECT_STEAMSHIPPINGID=480");
        GlobalDefinitions.Add("UE4_PROJECT_STEAMGAMEDIR=\"spacewar\"");
		GlobalDefinitions.Add("UE4_PROJECT_STEAMPRODUCTNAME=\"spacewar\"");
		GlobalDefinitions.Add("UE4_PROJECT_STEAMGAMEDESC=\"Projeto NOS\"");
	}
}
