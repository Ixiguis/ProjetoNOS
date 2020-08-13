// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.


#include "ShooterSpectatorPawn.h"

#define MIN_ZOOM 200.f
#define MAX_ZOOM 800.f
#define ZOOM_INCREMENT 40.f
#define DEFAULT_ZOOM 300.f

AShooterSpectatorPawn::AShooterSpectatorPawn()
{	
	bAllowFreeCam = false;
	bAllowSwitchFocus = false;

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	CameraBoom->TargetArmLength = 0.f;
	DesiredCameraDistance = 0.f;
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->AttachToComponent(CameraBoom, FAttachmentTransformRules::KeepRelativeTransform, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
}

void AShooterSpectatorPawn::SetupPlayerInputComponent(class UInputComponent* InInputComponent)
{
	check(InInputComponent);
	
	InInputComponent->BindAxis("MoveForward", this, &ADefaultPawn::MoveForward);
	InInputComponent->BindAxis("MoveRight", this, &ADefaultPawn::MoveRight);
	InInputComponent->BindAxis("MoveUp", this, &ADefaultPawn::MoveUp_World);
	InInputComponent->BindAxis("Turn", this, &ADefaultPawn::AddControllerYawInput);
	InInputComponent->BindAxis("TurnRate", this, &ADefaultPawn::TurnAtRate);
	InInputComponent->BindAxis("LookUp", this, &ADefaultPawn::AddControllerPitchInput);
	InInputComponent->BindAxis("LookUpRate", this, &AShooterSpectatorPawn::LookUpAtRate);
	
	InInputComponent->BindAction("NextWeapon", IE_Pressed, this, &AShooterSpectatorPawn::DecreaseZoom).bConsumeInput = false;
	InInputComponent->BindAction("PrevWeapon", IE_Pressed, this, &AShooterSpectatorPawn::IncreaseZoom).bConsumeInput = false;
	InInputComponent->BindAction("StartFire", IE_Pressed, this, &AShooterSpectatorPawn::SpectateNextCharacter).bConsumeInput = false;
	InInputComponent->BindAction("StartFire2", IE_Pressed, this, &AShooterSpectatorPawn::SpectatePreviousCharacter).bConsumeInput = false;
	InInputComponent->BindAction("StartJump", IE_Pressed, this, &AShooterSpectatorPawn::SpectateFreeCamera).bConsumeInput = false;
}

void AShooterSpectatorPawn::LookUpAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Val * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShooterSpectatorPawn::SpectateNextCharacter()
{
	if (!bAllowSwitchFocus)
	{
		return;
	}
	bool bFoundCurrentCharacter = false;
	for (TActorIterator<AShooterCharacter> It(GetWorld()); It; ++It)
	{
		AShooterCharacter* ShooterCharacter = *It;
		if (ShooterCharacter && ShooterCharacter->IsAlive())
		{
			if (bFoundCurrentCharacter)
			{
				SetViewTarget(ShooterCharacter);
				return;
			}
			if (ShooterCharacter == CurrentTarget)
			{
				bFoundCurrentCharacter = true;
			}
		}
	}
	//next character is NULL, set focus to first character again
	SetViewTarget(GetFirstCharacter());
}

AShooterCharacter* AShooterSpectatorPawn::GetFirstCharacter() const
{
	for (TActorIterator<AShooterCharacter> It(GetWorld()); It; ++It)
	{
		AShooterCharacter* ShooterCharacter = *It;
		if (ShooterCharacter && ShooterCharacter->IsAlive())
		{
			return ShooterCharacter;
		}
	}
	return NULL;
}

void AShooterSpectatorPawn::SpectatePreviousCharacter()
{
	if (!bAllowSwitchFocus)
	{
		return;
	}
	bool bFoundCurrentCharacter = false;
	AShooterCharacter* PreviousCharacter = GetLastCharacter();
	for (TActorIterator<AShooterCharacter> It(GetWorld()); It; ++It)
	{
		AShooterCharacter* ShooterCharacter = *It;
		if (ShooterCharacter && ShooterCharacter->IsAlive())
		{
			if (ShooterCharacter == CurrentTarget)
			{
				//found currently spectated character; keep PreviousCharacter and break this loop
				break;
			}
			PreviousCharacter = ShooterCharacter;
		}
	}
	SetViewTarget(PreviousCharacter);
}

AShooterCharacter* AShooterSpectatorPawn::GetLastCharacter() const
{
	AShooterCharacter* LastCharacter = NULL;
	for (TActorIterator<AShooterCharacter> It(GetWorld()); It; ++It)
	{
		AShooterCharacter* ShooterCharacter = *It;
		if (ShooterCharacter && ShooterCharacter->IsAlive())
		{
			LastCharacter = ShooterCharacter;
		}
	}
	return LastCharacter;
}

void AShooterSpectatorPawn::SpectateFreeCamera()
{
	if (bAllowFreeCam)
	{
		CameraBoom->TargetArmLength = 0.0f;
		RootComponent->SetWorldLocation(Camera->GetComponentLocation());
		CameraBoom->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		CurrentTarget = NULL;
	}
}

void AShooterSpectatorPawn::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AShooterSpectatorPawn::SetViewTarget(AActor* Target, bool bViewTargetIsDead)
{
	CurrentTarget = Target;
	if (Target != NULL)
	{
		CameraBoom->TargetArmLength = DEFAULT_ZOOM;
		DesiredCameraDistance = DEFAULT_ZOOM;
		AShooterCharacter* TargetCharacter = Cast<AShooterCharacter>(Target);
		if (TargetCharacter && (!TargetCharacter->IsAlive() || bViewTargetIsDead))
		{
			CameraBoom->AttachToComponent(TargetCharacter->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, FName("Heart"));
		}
		else
		{
			CameraBoom->AttachToComponent(Target->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		}
		//CameraBoom->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	}
	else
	{
		SpectateFreeCamera();
	}
}

void AShooterSpectatorPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsFreeCam())
	{
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, DesiredCameraDistance, DeltaSeconds, 2.f);
	}
}

void AShooterSpectatorPawn::IncreaseZoom()
{
	if (!IsFreeCam())
	{
		DesiredCameraDistance += ZOOM_INCREMENT;
		if (DesiredCameraDistance > MAX_ZOOM)
		{
			DesiredCameraDistance = MAX_ZOOM;
		}
	}
}

void AShooterSpectatorPawn::DecreaseZoom()
{
	if (!IsFreeCam())
	{
		DesiredCameraDistance -= ZOOM_INCREMENT;
		if (DesiredCameraDistance > MIN_ZOOM)
		{
			DesiredCameraDistance = MIN_ZOOM;
		}
	}
}
