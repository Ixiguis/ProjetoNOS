// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#include "GameRules/ShooterFlag.h"
#include "GameRules/ShooterFlagBase.h"


AShooterFlag::AShooterFlag()
{
	USceneComponent* SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneComponent;

	bReplicates = true;
	bAlwaysRelevant = true;

	CollisionComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionComponent"));
	CollisionComp->InitCapsuleSize(60.f, 80.f);
	CollisionComp->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	CollisionComp->AlwaysLoadOnClient = true;
	CollisionComp->AlwaysLoadOnServer = true;
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionObjectType(COLLISION_PICKUP);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComp->SetupAttachment(RootComponent);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComp->bCastDynamicShadow = true;
	MeshComp->SetRelativeLocation(FVector(0.f, 0.f, -80.f));
	MeshComp->bReceivesDecals = false;
	MeshComp->SetCollisionObjectType(ECC_WorldDynamic);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComp->SetupAttachment(CollisionComp);

	AutoReturnTime = 2.f;

	SetCanBeDamaged(false);
}


void AShooterFlag::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterFlag, TeamNumber);
	DOREPLIFETIME(AShooterFlag, IsDropped);
	DOREPLIFETIME(AShooterFlag, FlagCarrier); 
	DOREPLIFETIME(AShooterFlag, AutoReturnTime);
}

bool AShooterFlag::IsAtBase() const
{
	return FlagCarrier == NULL && !IsDropped;
}

bool AShooterFlag::TakeFlag(AShooterCharacter* Taker)
{
	if (Taker != NULL && Taker->IsAlive() && GetWorld()->GetTimeSeconds() - LastReturnTime > 0.5f)
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		RootComponent->AttachToComponent(Taker->GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
		FlagCarrier = Taker;
		if (IsDropped)
		{
			GetWorldTimerManager().ClearTimer(ReturnFlagHandle);
		}
		IsDropped = false;
		CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		return true;
	}
	return false;
}

void AShooterFlag::ReturnFlag()
{
	if (MyFlagBase != NULL)
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		RootComponent->AttachToComponent(MyFlagBase->GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
		FlagCarrier = NULL;
		if (IsDropped)
		{
			GetWorldTimerManager().ClearTimer(ReturnFlagHandle);
		}
		IsDropped = false;
		CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		LastReturnTime = GetWorld()->GetTimeSeconds();
	}
}

void AShooterFlag::DropFlag()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	//TODO: Checar se caiu em lava, fora do mundo, etc.
	IsDropped = true;
	FlagCarrier = NULL;
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	GetWorldTimerManager().SetTimer(ReturnFlagHandle, this, &AShooterFlag::ReturnFlag, AutoReturnTime, false);
}

void AShooterFlag::SetFlagBase(AShooterFlagBase* TheBase)
{
	MyFlagBase = TheBase;
	TeamNumber = TheBase->TeamNumber;
}
