// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Scoree.generated.h"

/**
 * 
 */
UCLASS()
class COMBATGAME_API UScoree : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Score")
	void UpdateScore(int Score);

protected:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* PlayerScore;
};
