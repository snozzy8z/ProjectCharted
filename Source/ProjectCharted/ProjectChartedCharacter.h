// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Weapon.h"
#include "Net/UnrealNetwork.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "ProjectChartedCharacter.generated.h"

/**
 * Personnage jouable C++ visible et sélectionnable dans l'éditeur Unreal Engine.
 */
UCLASS()
class PROJECTCHARTED_API AProjectChartedCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AProjectChartedCharacter();

    // Arme actuellement équipée (répliquée)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_CurrentWeapon, Category="Weapon")
    AWeapon* CurrentWeapon = nullptr;

    // Camera boom (bras de caméra)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    USpringArmComponent* CameraBoom;

    // Caméra de suivi (épaule droite)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    UCameraComponent* RightShoulderCamera;

    // Caméra épaule gauche
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    UCameraComponent* LeftShoulderCamera;

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION()
    void OnRep_CurrentWeapon();

    void AttachWeaponToSocket();
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    void OnFire();

    // --- ADS (Aim Down Sight) ---
    void OnAimPressed();
    void OnAimReleased();
    void UpdateAim(float DeltaTime);
    bool bIsAiming = false;
    float DefaultFOV = 90.f;
    float AimFOV = 58.f;
    float AimInterpSpeed = 10.f;
    float DefaultArmLength = 350.f;
    float AimArmLength = 160.f;
    FVector DefaultSocketOffset = FVector(0.f, 120.f, 60.f); // épaule droite
    FVector AimSocketOffset = FVector(50.f, 150.f, 68.f);   // visée très à droite
    // Offsets inspirés d'Uncharted 2 multi pour épaule gauche
    FVector DefaultSocketOffsetLeft = FVector(30.f, -75.f, 75.f); // épaule gauche, bien décalée et surélevée
    FVector AimSocketOffsetLeft = FVector(50.f, -80.f, 80.f);     // visée épaule gauche, plus proche et haute

    // Caméra épaule
    FVector DefaultCameraOffset = FVector(0.f, 0.f, 60.f); // Centré derrière le personnage
    FVector ShoulderCameraOffset = FVector(50.f, 60.f, 60.f);
    float CameraLerpSpeed = 10.f;
    void StartAiming();
    void StopAiming();
    void UpdateCameraShoulder(float DeltaTime);

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);

    // Saut
    void StartJump();
    void StopJump();

    // Changement d'épaule
    bool bIsRightShoulder = true;
    void ToggleShoulder();

public:

    // Fonction pour équiper une arme
    UFUNCTION(BlueprintCallable, Category="Weapon")
    void EquipWeapon(TSubclassOf<AWeapon> WeaponClass);

    // Fonction serveur pour ramasser une arme (bonus)
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerPickupWeapon(AActor* WeaponActor);

    /** Returns CameraBoom subobject **/
    FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

    /** Returns RightShoulderCamera subobject **/
    FORCEINLINE class UCameraComponent* GetRightShoulderCamera() const { return RightShoulderCamera; }
    
    /** Returns LeftShoulderCamera subobject **/
    FORCEINLINE class UCameraComponent* GetLeftShoulderCamera() const { return LeftShoulderCamera; }
};

