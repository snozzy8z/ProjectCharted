// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ProjectChartedPlayerController.generated.h"

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS()
class PROJECTCHARTED_API AProjectChartedPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
    // Suppression de SetupInputComponent (non nécessaire ici)
};
