// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "ShooterTypes.h"
#include "ShooterExplosionEffect.generated.h"

//
// Spawnable effect for explosion - NOT replicated to clients
// Each explosion type should be defined as separate blueprint
//
UCLASS(Abstract, Blueprintable)
class AShooterExplosionEffect : public AActor
{
	GENERATED_BODY()

public:

	AShooterExplosionEffect();

	/** explosion FX */
	UPROPERTY(EditDefaultsOnly, Category=Effect)
	UParticleSystem* ExplosionFX;

	/** explosion light */
	UPROPERTY(VisibleDefaultsOnly, Category=Effect)
	UPointLightComponent* ExplosionLight;

	/** how long keep explosion light on? */
	UPROPERTY(EditDefaultsOnly, Category=Effect)
	float ExplosionLightFadeOut;

	/** explosion sound */
	UPROPERTY(EditDefaultsOnly, Category=Effect)
	USoundCue* ExplosionSound;
	
	/** explosion decals */
	UPROPERTY(EditDefaultsOnly, Category=Effect)
	struct FDecalData Decal;

	/** surface data for spawning */
	UPROPERTY(BlueprintReadOnly, Category=Surface)
	FHitResult SurfaceHit;
	
	/** if true, explosion effects will always spawn on default rotation. Otherwise, uses impact normal's rotation. */
	UPROPERTY(EditDefaultsOnly, Category = Effect)
	bool ExplosionRotationAlwaysZero;

	/** spawn explosion */
	virtual void BeginPlay() override;

	/** update fading light */
	virtual void Tick(float DeltaSeconds) override;

private:

	/** Point light component name */
	FName ExplosionLightComponentName;
};
