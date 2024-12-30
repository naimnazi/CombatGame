// Fill out your copyright notice in the Description page of Project Settings.


#include "Scoree.h"
#include "Components/TextBlock.h"

void UScoree::UpdateScore(int Score)
{
	PlayerScore->SetText(FText::AsNumber(Score));
}