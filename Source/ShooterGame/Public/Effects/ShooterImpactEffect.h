// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "ShooterTypes.h"
#include "GameFramework/Actor.h"
#include "ShooterImpactEffect.generated.h"

//
// Spawnable effect for weapon hit impact - NOT replicated to clients
// Each impact type should be defined as separate blueprint
//
UCLASS(Abstract, Blueprintable)
class AShooterImpactEffect : public AActor
{
	GENERATED_BODY()

public:

	AShooterImpactEffect();

	/** default impact FX used when material specific override doesn't exist */
	UPROPERTY(EditDefaultsOnly, Category=Defaults)
	class UParticleSystem* DefaultFX;

	/** impact FX on concrete */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	class UParticleSystem* ConcreteFX;

	/** impact FX on dirt */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	class UParticleSystem* DirtFX;

	/** impact FX on water */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	class UParticleSystem* WaterFX;

	/** impact FX on metal */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	class UParticleSystem* MetalFX;

	/** impact FX on wood */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	class UParticleSystem* WoodFX;

	/** impact FX on glass */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	class UParticleSystem* GlassFX;

	/** impact FX on grass */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	class UParticleSystem* GrassFX;

	/** impact FX on flesh */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	class UParticleSystem* FleshFX;

	/** default impact sound used when material specific override doesn't exist */
	UPROPERTY(EditDefaultsOnly, Category=Defaults)
	class USoundCue* DefaultSound;

	/** impact FX on concrete */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	class USoundCue* ConcreteSound;

	/** impact FX on dirt */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	class USoundCue* DirtSound;

	/** impact FX on water */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	class USoundCue* WaterSound;

	/** impact FX on metal */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	class USoundCue* MetalSound;

	/** impact FX on wood */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	class USoundCue* WoodSound;

	/** impact FX on glass */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	class USoundCue* GlassSound;

	/** impact FX on grass */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	class USoundCue* GrassSound;

	/** impact FX on flesh */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	class USoundCue* FleshSound;

	/** default decal when material specific override doesn't exist */
	UPROPERTY(EditDefaultsOnly, Category=Defaults)
	struct FDecalData DefaultDecal;

	/** surface data for spawning */
	UPROPERTY(BlueprintReadOnly, Category=Surface)
	FHitResult SurfaceHit;

	/** spawn effect */
	virtual void PostInitializeComponents() override;

protected:

	/** get FX for material type */
	UParticleSystem* GetImpactFX(TEnumAsByte<EPhysicalSurface> SurfaceType) const;

	/** get sound for material type */
	USoundCue* GetImpactSound(TEnumAsByte<EPhysicalSurface> SurfaceType) const;
};
