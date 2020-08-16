// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

using UnrealBuildTool;

public class ShooterGame : ModuleRules
{
	public ShooterGame(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnforceIWYU = true;
        bFasterWithoutUnity = true;

		PrivateIncludePaths.AddRange(
			new string[] {
				"ShooterGame/Private",
				//"ShooterGame/Private/UI",
            }
		);


        PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"OnlineSubsystem",
				"OnlineSubsystemUtils",
				"AssetRegistry",
				"RHI",
                "AIModule",
                "GameplayTasks",
                "Json",
                "NavigationSystem",
            }
		);

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "InputCore",
			}
		);
        
        DynamicallyLoadedModuleNames.AddRange(
            new string[] {
                "OnlineSubsystemNull",
                "NetworkReplayStreaming",
				"NullNetworkReplayStreaming",
				"HttpNetworkReplayStreaming",
				"LocalFileNetworkReplayStreaming"
            }
        );

        PrivateIncludePathModuleNames.AddRange(
            new string[] {
                "NetworkReplayStreaming"
            }
        );

        if (Target.bBuildEditor == true)
        {
            PublicDependencyModuleNames.AddRange(
                new string[] {
                    "UnrealEd",
                }
            );
		}
    }

}
