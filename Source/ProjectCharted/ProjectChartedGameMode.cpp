// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectChartedGameMode.h"
#include "ProjectChartedCharacter.h"
#include "ProjectChartedPlayerController.h"

AProjectChartedGameMode::AProjectChartedGameMode()
{
	DefaultPawnClass = AProjectChartedCharacter::StaticClass();
	PlayerControllerClass = AProjectChartedPlayerController::StaticClass();
}
