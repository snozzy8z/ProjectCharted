// Copyright Epic Games, Inc. All Rights Reserved.

#include "Weapon.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/SkeletalMesh.h"

AWeapon::AWeapon()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(true);

    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    RootComponent = WeaponMesh;
    WeaponMesh->SetIsReplicated(true);

    // Chargement automatique du mesh
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshObj(TEXT("/Game/Weapons/AK47Subdiv.AK47Subdiv"));
    if (MeshObj.Succeeded())
    {
        WeaponMesh->SetSkeletalMesh(MeshObj.Object);
    }

    Damage = 20.0f;
}

void AWeapon::Fire()
{
    // Logique de tir à compléter (raycast, spawn projectile, etc.)
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AWeapon, Damage);
}
