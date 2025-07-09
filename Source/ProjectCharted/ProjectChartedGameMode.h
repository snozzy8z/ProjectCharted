// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ProjectChartedGameMode.generated.h"

class AProjectChartedPlayerController;
class AProjectChartedCharacter;

/**
 *  Simple GameMode for a third person game
 */
UCLASS()
class PROJECTCHARTED_API AProjectChartedGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AProjectChartedGameMode();
};



