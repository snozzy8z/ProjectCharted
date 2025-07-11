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
 * Personnage jouable C++ visible et s�lectionnable dans l'�diteur Unreal Engine.
 */
UCLASS()
class PROJECTCHARTED_API AProjectChartedCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AProjectChartedCharacter();

    // Arme actuellement �quip�e (r�pliqu�e)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_CurrentWeapon, Category="Weapon")
    AWeapon* CurrentWeapon = nullptr;

    // Camera boom (bras de cam�ra)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    USpringArmComponent* CameraBoom;

    // Cam�ra de suivi (�paule droite)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    UCameraComponent* RightShoulderCamera;

    // Cam�ra �paule gauche
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
    FVector DefaultSocketOffset = FVector(0.f, 120.f, 60.f); // �paule droite
    FVector AimSocketOffset = FVector(50.f, 150.f, 68.f);   // vis�e tr�s � droite
    // Offsets inspir�s d'Uncharted 2 multi pour �paule gauche
    FVector DefaultSocketOffsetLeft = FVector(30.f, -75.f, 75.f); // �paule gauche, bien d�cal�e et sur�lev�e
    FVector AimSocketOffsetLeft = FVector(50.f, -80.f, 80.f);     // vis�e �paule gauche, plus proche et haute

    // Cam�ra �paule
    FVector DefaultCameraOffset = FVector(0.f, 0.f, 60.f); // Centr� derri�re le personnage
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

    // Changement d'�paule
    bool bIsRightShoulder = true;
    void ToggleShoulder();

public:

    // Fonction pour �quiper une arme
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

