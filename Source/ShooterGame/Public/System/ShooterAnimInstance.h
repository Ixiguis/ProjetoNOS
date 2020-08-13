// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.  

#pragma once

#include "Animation/AnimInstance.h"
#include "ShooterAnimInstance.generated.h"

/**
*
*/
UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType)
class UShooterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UShooterAnimInstance();

	//	virtual void AddAnimNotifies(const TArray<const FAnimNotifyEvent*>& NewNotifies, const float InstanceWeight) OVERRIDE;

	/** bIsMesh1PBody has head and arms hidden */
	UPROPERTY(BlueprintReadWrite, Category = Animation)
	bool bIsMesh1PBody;

	/** if false, this anim blueprint won't process any anim notifies */
	UPROPERTY(BlueprintReadWrite, Category = Animation)
	uint32 bProcessAnimNotifies : 1;
};
