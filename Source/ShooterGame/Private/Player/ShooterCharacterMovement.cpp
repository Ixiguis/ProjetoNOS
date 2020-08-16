// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "Player/ShooterCharacterMovement.h"
#include "Player/ShooterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundCue.h"

UShooterCharacterMovement::UShooterCharacterMovement()
{
	NavAgentProps.bCanCrouch = true;
	NavAgentProps.bCanSwim = true;
	bAlwaysCheckFloor = true;
	Buoyancy = 1.0f;
	SoulHunterSpeedMod = 1.f;
	PowerupSpeedMod = 1.f;
	EnvironmentSpeedMod = 1.f;
	GameModeSpeedMod = 1.f;
}

void UShooterCharacterMovement::InitializeComponent()
{
	Super::InitializeComponent();
	DefaultMaxWalkSpeed = MaxWalkSpeed;
	DefaultMaxAcceleration = MaxAcceleration;
}

bool UShooterCharacterMovement::CanCrouchInCurrentState() const 
{
	return MovementMode == MOVE_Walking;
}

bool UShooterCharacterMovement::DoJump(bool bReplayingMoves)
{
	if (ShooterCharacterOwner)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShooterCharacterOwner->JumpSound, ShooterCharacterOwner->GetActorLocation());
		//AddMomentum(FVector(0.f, 0.f, JumpZVelocity), true);// +FVector(0.f, 0.f, CharacterVelocity.Z), true);
		AddImpulse(FVector(0.f, 0.f, JumpZVelocity), true);
		return true;
	}
	return false;
}

void UShooterCharacterMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	const FVector PreviousLoc = GetActorFeetLocation();
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const FVector NewLoc = GetActorFeetLocation();
	CharacterVelocity = (NewLoc - PreviousLoc) / DeltaTime;

	if (ShooterCharacterOwner)
	{
		const float DispersionChange = CharacterVelocity.SizeSquared() * 0.000001;
		ShooterCharacterOwner->IncreaseAimingDispersion(DispersionChange);
	}
}

void UShooterCharacterMovement::SetUpdatedComponent(USceneComponent* NewUpdatedComponent)
{
	Super::SetUpdatedComponent(NewUpdatedComponent);
	ShooterCharacterOwner = Cast<AShooterCharacter>(PawnOwner);
}

void UShooterCharacterMovement::SetSoulHunterSpeedMod(float NewMod)
{
	if (PawnOwner && PawnOwner->GetLocalRole() == ROLE_Authority)
	{
		SoulHunterSpeedMod = NewMod;
		OnRep_SpeedModifier();
	}
}

void UShooterCharacterMovement::SetPowerupSpeedMod(float NewMod)
{
	if (PawnOwner && PawnOwner->GetLocalRole() == ROLE_Authority)
	{
		PowerupSpeedMod = NewMod;
		OnRep_SpeedModifier();
	}
}

void UShooterCharacterMovement::SetEnvironmentSpeedMod(float NewMod)
{
	if (PawnOwner && PawnOwner->GetLocalRole() == ROLE_Authority)
	{
		EnvironmentSpeedMod = NewMod;
		OnRep_SpeedModifier();
	}
}

void UShooterCharacterMovement::SetGameModeSpeedMod(float NewMod)
{
	if (PawnOwner && PawnOwner->GetLocalRole() == ROLE_Authority)
	{
		GameModeSpeedMod = NewMod;
		OnRep_SpeedModifier();
	}
}

void UShooterCharacterMovement::OnRep_SpeedModifier()
{
	MaxWalkSpeed = DefaultMaxWalkSpeed * SoulHunterSpeedMod * PowerupSpeedMod * EnvironmentSpeedMod * GameModeSpeedMod;
	MaxAcceleration = MaxWalkSpeed / DefaultMaxWalkSpeed * DefaultMaxAcceleration;
}

void UShooterCharacterMovement::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UShooterCharacterMovement, SoulHunterSpeedMod);
	DOREPLIFETIME(UShooterCharacterMovement, PowerupSpeedMod);
	DOREPLIFETIME(UShooterCharacterMovement, EnvironmentSpeedMod);
	DOREPLIFETIME(UShooterCharacterMovement, GameModeSpeedMod);
}