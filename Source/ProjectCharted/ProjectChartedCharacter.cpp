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
    DefaultArmLength = 350.0f; // Un peu plus loin par défaut
    AimArmLength = 160.0f;    // Plus proche en visée
    CameraBoom->TargetArmLength = DefaultArmLength;
    CameraBoom->bUsePawnControlRotation = true;
    // Nouvelle position centrée derrière le personnage
    DefaultSocketOffset = FVector(30.f, 75.f, 75.f); // Y=75 pour caméra épaule droite (plus proche de la gauche)
    AimSocketOffset = FVector(50.f, 80.f, 80.f);    // Y=80 pour visée à droite, Z=80 pour surélever
    DefaultSocketOffsetLeft = FVector(30.f, -75.f, 75.f); // Y=-75 pour caméra épaule gauche
    AimSocketOffsetLeft = FVector(50.f, -80.f, 80.f);    // Y=-80 pour visée à gauche, Z=80 pour surélever
    CameraBoom->SocketOffset = DefaultSocketOffset;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 10.0f;

    // Camera épaule droite
    RightShoulderCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("RightShoulderCamera"));
    RightShoulderCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    DefaultFOV = 90.0f;
    AimFOV = 58.0f; // FOV plus serré en visée
    RightShoulderCamera->FieldOfView = DefaultFOV;
    RightShoulderCamera->bUsePawnControlRotation = false;
    RightShoulderCamera->SetActive(true);

    // Camera épaule gauche
    LeftShoulderCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("LeftShoulderCamera"));
    LeftShoulderCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    LeftShoulderCamera->FieldOfView = DefaultFOV;
    LeftShoulderCamera->bUsePawnControlRotation = false;
    LeftShoulderCamera->SetActive(false);

    bIsAiming = false;
    AimInterpSpeed = 10.0f;

    // Mouvement
    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);

    // Mouvement fluide orienté vers la direction du déplacement (style Uncharted)
    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f); // Rotation rapide et naturelle

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

    // Affecte le mesh squelette du personnage (SKM_Manny_Simple)
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshObj(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
    if (MeshObj.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(MeshObj.Object);
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
        GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
        // Affecte le Blueprint d'animation Unarmed
        static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBPClass(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"));
        if (AnimBPClass.Succeeded())
        {
            GetMesh()->SetAnimInstanceClass(AnimBPClass.Class);
        }
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
    if (RightShoulderCamera)
    {
        DefaultFOV = RightShoulderCamera->FieldOfView;
    }
}

void AProjectChartedCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateCameraShoulder(DeltaTime);
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

    PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &AProjectChartedCharacter::StartAiming);
    PlayerInputComponent->BindAction("Aim", IE_Released, this, &AProjectChartedCharacter::StopAiming);

    PlayerInputComponent->BindAction("ToggleShoulder", IE_Pressed, this, &AProjectChartedCharacter::ToggleShoulder);
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
    bIsRightShoulder = true; // Revient à l'épaule droite
}

void AProjectChartedCharacter::UpdateAim(float DeltaTime)
{
    if (CameraBoom && RightShoulderCamera && LeftShoulderCamera)
    {
        float TargetArmLength = bIsAiming ? AimArmLength : DefaultArmLength;
        float TargetFOV = bIsAiming ? AimFOV : DefaultFOV;
        FVector TargetOffset;
        if (bIsRightShoulder)
        {
            TargetOffset = bIsAiming ? AimSocketOffset : DefaultSocketOffset;
        }
        else
        {
            TargetOffset = bIsAiming ? AimSocketOffsetLeft : DefaultSocketOffsetLeft;
        }
        CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArmLength, DeltaTime, AimInterpSpeed);
        FVector Offset = CameraBoom->SocketOffset;
        Offset.X = FMath::FInterpTo(Offset.X, TargetOffset.X, DeltaTime, AimInterpSpeed);
        Offset.Y = FMath::FInterpTo(Offset.Y, TargetOffset.Y, DeltaTime, AimInterpSpeed);
        Offset.Z = FMath::FInterpTo(Offset.Z, TargetOffset.Z, DeltaTime, AimInterpSpeed);
        CameraBoom->SocketOffset = Offset;

        // Active la bonne caméra
        if (bIsRightShoulder)
        {
            RightShoulderCamera->SetActive(true);
            LeftShoulderCamera->SetActive(false);
            RightShoulderCamera->SetFieldOfView(FMath::FInterpTo(RightShoulderCamera->FieldOfView, TargetFOV, DeltaTime, AimInterpSpeed));
        }
        else
        {
            RightShoulderCamera->SetActive(false);
            LeftShoulderCamera->SetActive(true);
            LeftShoulderCamera->SetFieldOfView(FMath::FInterpTo(LeftShoulderCamera->FieldOfView, TargetFOV, DeltaTime, AimInterpSpeed));
        }
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
    AddControllerPitchInput(-Value); // Inversion de l'axe Y pour la souris
}

void AProjectChartedCharacter::StartJump()
{
    Jump();
}

void AProjectChartedCharacter::StopJump()
{
    StopJumping();
}

void AProjectChartedCharacter::StartAiming()
{
    bIsAiming = true;
}

void AProjectChartedCharacter::StopAiming()
{
    bIsAiming = false;
}

void AProjectChartedCharacter::UpdateCameraShoulder(float DeltaTime)
{
    if (CameraBoom)
    {
        FVector TargetOffset = bIsAiming ? ShoulderCameraOffset : DefaultCameraOffset;
        FVector CurrentOffset = CameraBoom->SocketOffset;
        FVector NewOffset;
        NewOffset.X = FMath::FInterpTo(CurrentOffset.X, TargetOffset.X, DeltaTime, CameraLerpSpeed);
        NewOffset.Y = FMath::FInterpTo(CurrentOffset.Y, TargetOffset.Y, DeltaTime, CameraLerpSpeed);
        NewOffset.Z = FMath::FInterpTo(CurrentOffset.Z, TargetOffset.Z, DeltaTime, CameraLerpSpeed);
        CameraBoom->SocketOffset = NewOffset;
    }
}

void AProjectChartedCharacter::ToggleShoulder()
{
    if (bIsAiming) // Ne change d'épaule que si on vise
    {
        bIsRightShoulder = !bIsRightShoulder;
    }
}
