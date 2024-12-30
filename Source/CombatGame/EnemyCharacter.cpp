// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"
#include "GameFramework/Actor.h"
#include "Animation/AnimMontage.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"  // For physics-based knockback
#include "Components/CapsuleComponent.h"  // For capsule collision
#include "Components/SkeletalMeshComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Components/WidgetComponent.h"
#include "HealthBar.h"
#include "EngineUtils.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"
#include "CombatGameCharacter.h"
#include "Net/UnrealNetwork.h"


// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  
	PrimaryActorTick.bCanEverTick = true;
	// Set default health values
	//MaxHealth = 100.f; // Set the max health to 100 (can be adjusted)
	//Health = MaxHealth; // Set the current health to max health by default
	HealthWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	HealthWidgetComp->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	Health = 100;
	FireEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("FireEffect"));
	FireEffect->SetupAttachment(GetMesh()); // Attach to the skeletal mesh
	FireEffect->bAutoActivate = false;      // Disable auto-activation
}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	// Enable physics for the mesh
	//GetMesh()->SetSimulatePhysics(true);
	//GetMesh()->SetEnableGravity(true); // Enable gravity 
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);  // Enable query and physics for mesh
	GetMesh()->SetCollisionResponseToAllChannels(ECR_Block);  // Default behavior for all channels
	GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);  // Block pawn
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);  // Block world static (ground, walls)
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);  // Block dynamic world objects
	UpdateHealthBar(Health, 100);
	FireEffect->Activate();
	// Set up a timer to periodically find and move to the closest player
	GetWorld()->GetTimerManager().SetTimer(
		MovementTimerHandle,
		this,
		&AEnemyCharacter::MoveToClosestPlayer,
		1.0f, // Repeat interval in seconds
		true  // Loop the timer
	);
}

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//if (gerak)
	//{
	//	//MoveTowardsPlayer();
	//	MoveToClosestPlayer();
	//}
	
}

// Called to bind functionality to input
void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}


void AEnemyCharacter::Test(FVector HitDirection, int Damage)
{
	UE_LOG(LogTemp, Warning, TEXT("test"));
	/*if (HitAnimationMontage)
		{
			GetMesh()->PlayAnimation(HitAnimationMontage, false);
		}*/
	
	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();  // Get the CapsuleComponent for launching
	//USkeletalMeshComponent* CharacterMesh = GetMesh(); // Get the StaticMeshComponent for physics simulation

	if (CapsuleComp)
	{
		
		FVector LaunchDirection = HitDirection.GetSafeNormal();

	
		LaunchDirection.Z = FMath::Clamp(LaunchDirection.Z, 0.3f, 1.0f); // Adjust vertical 

		// how far you want the enemy to be thrown
		float LaunchStrength = 1500.0f;

		// Calculate the final launch velocity.
		FVector LaunchVelocity = LaunchDirection * LaunchStrength;

		// Apply the launch
		LaunchCharacter(LaunchVelocity,true, true);
		Health -= Damage;
		UpdateHealthBar(Health, 100);
		if (Health <= 0)
		{
			Die();
			FireEffect->Deactivate();
		}
	}
}
	
void AEnemyCharacter::Die()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh(); // Get the StaticMeshComponent for physics simulation
	if (CharacterMesh)
	{
		// Use a delay or timer to give the capsule time to move before enabling physics
		GetWorld()->GetTimerManager().SetTimerForNextTick([=]()
			{
				CharacterMesh->SetSimulatePhysics(true);  // Enable physics for the static mesh, it will fall to the ground
				
			});
		//gerak = false;
		// Stop movement by clearing the timer
		GetWorld()->GetTimerManager().ClearTimer(MovementTimerHandle);
		AAIController* AIController = Cast<AAIController>(GetController());
		
		if (HealthWidgetComp && AIController)
		{
			//AIController->UnPossess();
			AIController->StopMovement();
			HealthWidgetComp->DestroyComponent();
			HealthWidgetComp = nullptr;
			ACombatGameCharacter* PlayerCharacter = Cast<ACombatGameCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
			if (PlayerCharacter)
			{
				PlayerCharacter->UpdateScore(20);
			}
		}
		
		
	}
	
}

//void AEnemyCharacter::MoveTowardsPlayer()
//{
//	// Get the AI Controller
//	AAIController* AIController = Cast<AAIController>(GetController());
//	if (AIController)
//	{
//		// Find the player's location
//		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
//		if (PlayerController)
//		{
//			APawn* PlayerPawn = PlayerController->GetPawn();
//			if (PlayerPawn)
//			{
//				// Move towards the player
//				GetCharacterMovement()->MaxWalkSpeed = 400.0f;
//				AIController->MoveToActor(PlayerPawn);
//				//UE_LOG(LogTemp, Warning, TEXT("kejar player"));
//				//UE_LOG(LogTemp, Warning, TEXT("Moving towards player at location: %s"), *PlayerPawn->GetActorLocation().ToString());
//				
//			}
//		}
//	}
//}

void AEnemyCharacter::UpdateHealthBar(float CurrentHealth, float MaxHealth)
{
	
	if (UUserWidget* Widget = HealthWidgetComp->GetUserWidgetObject())
	{
		UHealthBar* HealthBar = Cast<UHealthBar>(Widget);
		if (HealthBar)
		{
			// Update the health bar widget
			HealthBar->UpdateHealth(CurrentHealth, MaxHealth);
		}
	}
}

void AEnemyCharacter::MoveToClosestPlayer()
{
	// Get the AIController
	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Enemy AIController not found!"));
		return;
	}

	// Find the closest player
	APawn* ClosestPlayer = nullptr;
	float MinDistance = FLT_MAX;

	// Iterate through all player controllers
	for (APlayerController* PlayerController : TActorRange<APlayerController>(GetWorld()))
	{
		APawn* PlayerPawn = PlayerController->GetPawn();
		if (PlayerPawn)
		{
			float Distance = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());
			if (Distance < MinDistance)
			{
				MinDistance = Distance;
				ClosestPlayer = PlayerPawn;
			}
		}
	}

	// If a player is found, move to them
	if (ClosestPlayer)
	{
		GetCharacterMovement()->MaxWalkSpeed = 250.0f;
		AIController->MoveToActor(ClosestPlayer);
		//UE_LOG(LogTemp, Warning, TEXT("Enemy is moving to player: %s"), *ClosestPlayer->GetName());
	}
}

void AEnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEnemyCharacter, Health);
}
void AEnemyCharacter::OnRep_Health()
{
	UE_LOG(LogTemp, Log, TEXT("Health: replicate masukk"));
	UpdateHealthBar(Health,100);
	if (Health <= 0)
	{
		Die();
		FireEffect->Deactivate();
	}
}


