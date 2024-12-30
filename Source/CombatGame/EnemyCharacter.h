// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class AAIController;

UCLASS()
class COMBATGAME_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyCharacter();
	// Function to apply impulse when hit
	//void ApplyKnockback(FVector Direction, float Strength);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	// Declare the animation montage for the hit animation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	UAnimMontage* HitAnimationMontage;

	UPROPERTY(VisibleAnywhere)
	class UWidgetComponent* HealthWidgetComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effects")
	class UParticleSystemComponent* FireEffect;

public:
	void Test(FVector HitDirection, int Damage);
	//void DisablePhysics();
	FTimerHandle PhysicsDisableTimerHandle;
	FTimerHandle MovementTimerHandle;
	//void MoveTowardsPlayer();
	//void ApplyKnockback(FVector HitDirection);
	//bool gerak = true;
	void Die();

	UPROPERTY(ReplicatedUsing = OnRep_Health)
	float Health;

	UFUNCTION()
	void OnRep_Health();
	// Update health bar
	void UpdateHealthBar(float CurrentHealth, float MaxHealth);

private:
	void MoveToClosestPlayer();
	//FTimerHandle TimerHandle_FindClosestPlayer;
};
