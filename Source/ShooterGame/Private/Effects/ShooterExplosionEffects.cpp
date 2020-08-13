// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "ShooterGame.h"
#include "ShooterExplosionEffect.h"

AShooterExplosionEffect::AShooterExplosionEffect()
{
	ExplosionLightComponentName = TEXT("ExplosionLight");

	PrimaryActorTick.bCanEverTick = true;

	ExplosionLight = CreateDefaultSubobject<UPointLightComponent>(ExplosionLightComponentName);
	ExplosionLight->AttenuationRadius = 400.0;
	ExplosionLight->Intensity = 500.0f;
	ExplosionLight->bUseInverseSquaredFalloff = false;
	ExplosionLight->LightColor = FColor(255, 185, 35);
	ExplosionLight->CastShadows = false;
	ExplosionLight->SetVisibility(true);

	ExplosionRotationAlwaysZero = false;
	ExplosionLightFadeOut = 0.2f;

	Decal.LifeSpan = 30.f;
}

void AShooterExplosionEffect::BeginPlay()
{
	Super::BeginPlay();

	const FVector EffectLocation = SurfaceHit.ImpactPoint;
	if (ExplosionFX)
	{
		FRotator ParticleRotation = FRotator::ZeroRotator;
		if (!ExplosionRotationAlwaysZero)
		{
			ParticleRotation = SurfaceHit.ImpactNormal.Rotation();
			//change pitch so that the particle is Z-up with the impact normal
			ParticleRotation.Pitch -= 90.f;
		}
		UGameplayStatics::SpawnEmitterAtLocation(this, ExplosionFX, EffectLocation, ParticleRotation);
	}

	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, EffectLocation);
	}

	if (Decal.DecalMaterial)
	{
		FRotator RandomDecalRotation = SurfaceHit.ImpactNormal.Rotation();
		RandomDecalRotation.Roll = FMath::FRandRange(-180.0f, 180.0f);

		UPrimitiveComponent* HitComp = SurfaceHit.Component.Get();

		if (HitComp && Cast<APawn>(SurfaceHit.GetActor()) == NULL)
		{
			UGameplayStatics::SpawnDecalAttached(Decal.DecalMaterial, FVector(Decal.DecalSize, Decal.DecalSize, 1.0f),
				HitComp, SurfaceHit.BoneName,
				SurfaceHit.ImpactPoint, RandomDecalRotation, EAttachLocation::KeepWorldPosition,
				Decal.LifeSpan);
		}
	}
}

void AShooterExplosionEffect::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float TimeAlive = GetWorld()->GetTimeSeconds() - CreationTime;
	const float TimeRemaining = FMath::Max(0.0f, ExplosionLightFadeOut - TimeAlive);

	if (TimeRemaining > 0)
	{
		const float FadeAlpha = 1.0f - FMath::Square(TimeRemaining / ExplosionLightFadeOut);

		UPointLightComponent* DefLight = Cast<UPointLightComponent>(GetClass()->GetDefaultSubobjectByName(ExplosionLightComponentName));
		ExplosionLight->SetIntensity(DefLight->Intensity * FadeAlpha);
	}
	else
	{
		Destroy();
	}
}
