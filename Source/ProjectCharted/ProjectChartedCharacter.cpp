// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectChartedCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/SkeletalMesh.h"

AProjectChartedCharacter::AProjectChartedCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	bReplicates = true;
	CurrentWeapon = nullptr;

	// Affecte le mesh squelette du personnage
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshObj(TEXT("/Game/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin"));
	if (MeshObj.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MeshObj.Object);
		// Optionnel : ajuste la position/rotation du mesh si besoin
		GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
		GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	}

    // --- ADS (Aim Down Sight) ---
    DefaultFOV = 90.f;
    AimFOV = 60.f;
    AimInterpSpeed = 10.f;
    bIsAiming = false;
}

void AProjectChartedCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && !CurrentWeapon)
	{
		// Spawn l'arme côté serveur uniquement
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		CurrentWeapon = GetWorld()->SpawnActor<AWeapon>(AWeapon::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (CurrentWeapon)
		{
			AttachWeaponToSocket();
		}
	}
	else if (CurrentWeapon)
	{
		AttachWeaponToSocket();
	}

    // Initialiser le FOV par défaut
    if (FollowCamera)
    {
        DefaultFOV = FollowCamera->FieldOfView;
    }
}

void AProjectChartedCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AProjectChartedCharacter::OnFire);
    PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &AProjectChartedCharacter::OnAimPressed);
    PlayerInputComponent->BindAction("Aim", IE_Released, this, &AProjectChartedCharacter::OnAimReleased);
}

// --- Réplication ---
void AProjectChartedCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AProjectChartedCharacter, CurrentWeapon);
}

// --- Equipement d'une arme ---
void AProjectChartedCharacter::EquipWeapon(TSubclassOf<AWeapon> WeaponClass)
{
    if (!HasAuthority() || !WeaponClass) return;

    // Détruire l'arme actuelle si besoin
    if (CurrentWeapon)
    {
        CurrentWeapon->Destroy();
        CurrentWeapon = nullptr;
    }

    // Spawn la nouvelle arme
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    CurrentWeapon = GetWorld()->SpawnActor<AWeapon>(WeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    if (CurrentWeapon)
    {
        AttachWeaponToSocket();
    }
}

void AProjectChartedCharacter::AttachWeaponToSocket()
{
    if (CurrentWeapon && GetMesh())
    {
        CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("WeaponSocket"));
    }
}

void AProjectChartedCharacter::OnRep_CurrentWeapon()
{
    AttachWeaponToSocket();
}

// --- Input Fire ---
void AProjectChartedCharacter::OnFire()
{
    if (CurrentWeapon)
    {
        CurrentWeapon->Fire();
    }
}

void AProjectChartedCharacter::OnAimPressed()
{
    bIsAiming = true;
    // Optionnel : ralentir la vitesse de déplacement ici
    GetCharacterMovement()->MaxWalkSpeed = 250.f;
}

void AProjectChartedCharacter::OnAimReleased()
{
    bIsAiming = false;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
}

void AProjectChartedCharacter::UpdateAim(float DeltaTime)
{
    if (FollowCamera)
    {
        float TargetFOV = bIsAiming ? AimFOV : DefaultFOV;
        float NewFOV = FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaTime, AimInterpSpeed);
        FollowCamera->SetFieldOfView(NewFOV);
    }
}

void AProjectChartedCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateAim(DeltaTime);
}

// --- Bonus : Ramasser une arme au sol ---
void AProjectChartedCharacter::ServerPickupWeapon_Implementation(AActor* WeaponActor)
{
    if (!HasAuthority() || !WeaponActor) return;
    AWeapon* PickupWeapon = Cast<AWeapon>(WeaponActor);
    if (PickupWeapon)
    {
        // Détruire l'arme actuelle
        if (CurrentWeapon)
        {
            CurrentWeapon->Destroy();
            CurrentWeapon = nullptr;
        }
        // Attacher l'arme ramassée
        CurrentWeapon = PickupWeapon;
        AttachWeaponToSocket();
        // Désactiver la collision pour éviter de la ramasser à nouveau
        PickupWeapon->SetActorEnableCollision(false);
    }
}
bool AProjectChartedCharacter::ServerPickupWeapon_Validate(AActor* WeaponActor) { return true; }
