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
#include "Components/InputComponent.h"

AProjectChartedCharacter::AProjectChartedCharacter()
{
    // Capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // Camera boom
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 300.0f;
    CameraBoom->bUsePawnControlRotation = true;

    // Camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // Mouvement
    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);

    // ADS
    bIsAiming = false;
    DefaultFOV = 90.f;
    AimFOV = 60.f;
    AimInterpSpeed = 10.f;

    // Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
    // instead of recompiling to adjust them
    GetCharacterMovement()->JumpZVelocity = 500.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

    // Affecte le mesh squelette du personnage (nouveau chemin)
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshObj(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
    if (MeshObj.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(MeshObj.Object);
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
        GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
    }

    bReplicates = true;
    CurrentWeapon = nullptr;
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

void AProjectChartedCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateAim(DeltaTime);
}

void AProjectChartedCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // Déplacement ZQSD/WASD
    PlayerInputComponent->BindAxis("MoveForward", this, &AProjectChartedCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &AProjectChartedCharacter::MoveRight);

    // Caméra souris
    PlayerInputComponent->BindAxis("Turn", this, &AProjectChartedCharacter::Turn);
    PlayerInputComponent->BindAxis("LookUp", this, &AProjectChartedCharacter::LookUp);

    // Saut
    PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AProjectChartedCharacter::StartJump);
    PlayerInputComponent->BindAction("Jump", IE_Released, this, &AProjectChartedCharacter::StopJump);

    // Visée (clic droit)
    PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &AProjectChartedCharacter::OnAimPressed);
    PlayerInputComponent->BindAction("Aim", IE_Released, this, &AProjectChartedCharacter::OnAimReleased);

    // Tir (clic gauche)
    PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AProjectChartedCharacter::OnFire);
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

void AProjectChartedCharacter::MoveForward(float Value)
{
    if (Controller && Value != 0.0f)
    {
        const FRotator YawRotation(0, Controller->GetControlRotation().Yaw, 0);
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        AddMovementInput(Direction, Value);
    }
}

void AProjectChartedCharacter::MoveRight(float Value)
{
    if (Controller && Value != 0.0f)
    {
        const FRotator YawRotation(0, Controller->GetControlRotation().Yaw, 0);
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        AddMovementInput(Direction, Value);
    }
}

void AProjectChartedCharacter::Turn(float Value)
{
    AddControllerYawInput(Value);
}

void AProjectChartedCharacter::LookUp(float Value)
{
    AddControllerPitchInput(Value);
}

void AProjectChartedCharacter::StartJump()
{
    Jump();
}

void AProjectChartedCharacter::StopJump()
{
    StopJumping();
}
