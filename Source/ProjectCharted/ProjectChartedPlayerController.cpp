// Copyright Epic Games, Inc. All Rights Reserved.


#include "ProjectChartedPlayerController.h"

void AProjectChartedPlayerController::BeginPlay()
{
	Super::BeginPlay();
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	SetInputMode(FInputModeGameOnly());
}

// Suppression de SetupInputComponent (non nécessaire ici)
