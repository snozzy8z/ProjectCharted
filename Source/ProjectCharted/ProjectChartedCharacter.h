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

    // Caméra de suivi
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    UCameraComponent* FollowCamera;

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
    float AimFOV = 60.f;
    float AimInterpSpeed = 10.f;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);

    // Saut
    void StartJump();
    void StopJump();

public:

    // Fonction pour équiper une arme
    UFUNCTION(BlueprintCallable, Category="Weapon")
    void EquipWeapon(TSubclassOf<AWeapon> WeaponClass);

    // Fonction serveur pour ramasser une arme (bonus)
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerPickupWeapon(AActor* WeaponActor);

    /** Returns CameraBoom subobject **/
    FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

    /** Returns FollowCamera subobject **/
    FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

