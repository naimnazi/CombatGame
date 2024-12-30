// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthBar.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UHealthBar::UpdateHealth(float CurrentHealth, float MaxHealth)
{
    if (HealthBar)
    {
        // Set the health percentage for the progress bar
        float HealthPercent = MaxHealth > 0 ? CurrentHealth / MaxHealth : 0.0f;
        HealthBar->SetPercent(HealthPercent);
    }

    if (CurrentHealthLabel)
    {
        // Update the current health text
        CurrentHealthLabel->SetText(FText::AsNumber(FMath::RoundToInt(CurrentHealth)));
    }

    if (MaxHealthLabel)
    {
        // Update the max health text
        MaxHealthLabel->SetText(FText::AsNumber(FMath::RoundToInt(MaxHealth)));
    }
}

