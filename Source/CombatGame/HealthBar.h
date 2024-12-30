// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "HealthBar.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class COMBATGAME_API UHealthBar : public UUserWidget
{
	GENERATED_BODY()

public:
	// Function to update health values in the widget
	UFUNCTION(BlueprintCallable, Category = "Health")
	void UpdateHealth(float CurrentHealth, float MaxHealth);
	
protected:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* CurrentHealthLabel;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* MaxHealthLabel;

};
