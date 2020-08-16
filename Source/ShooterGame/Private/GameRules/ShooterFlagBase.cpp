// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#include "GameRules/ShooterFlagBase.h"
#include "Components/BillboardComponent.h"
#include "UObject/ConstructorHelpers.h"

AShooterFlagBase::AShooterFlagBase()
{
	TeamNumber = 0;

	USceneComponent* SceneComp = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComp"));
	RootComponent = SceneComp;

	SpriteComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (!IsRunningCommandlet())
	{
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> TextureObject;
			FName ID_Misc;
			FText NAME_Misc;
			FConstructorStatics()
				: TextureObject(TEXT("/Game/UI/Editor/FlagEditor"))
				, ID_Misc(TEXT("Misc"))
				, NAME_Misc(NSLOCTEXT("SpriteCategory", "Misc", "Misc"))
			{
			}
		};
		static FConstructorStatics ConstructorStatics;

		SpriteComponent->Sprite = ConstructorStatics.TextureObject.Get();
		SpriteComponent->bHiddenInGame = true;
		SpriteComponent->SetupAttachment(SceneComp);
		SpriteComponent->SetRelativeScale3D(FVector(2.f));
		SpriteComponent->SetRelativeLocation(FVector(0.f, 0.f, 140.f));
		SpriteComponent->bIsScreenSizeScaled = false;
//		SpriteComponent->Sprite->AdjustHue = GTeamColors[TeamNumber].ReinterpretAsLinear().LinearRGBToHSV().R;
//		SpriteComponent->Sprite->AdjustSaturation = 1.8f;
//		SpriteComponent->Sprite->AdjustVibrance = 1.f;
	}

}


