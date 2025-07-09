// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Weapon.generated.h"

/**
 * Classe d'arme de base pour le multijoueur (h�rite de AActor)
 */
UCLASS()
class AWeapon : public AActor
{
    GENERATED_BODY()

public:
    // Constructeur
    AWeapon();

protected:
    // Mesh squelette de l'arme (root)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    USkeletalMeshComponent* WeaponMesh;

    // D�g�ts de l'arme
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Weapon")
    float Damage;

public:
    // Fonction appel�e pour tirer
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    virtual void Fire();

    // R�plication
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
