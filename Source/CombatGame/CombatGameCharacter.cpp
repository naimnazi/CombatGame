// Copyright Epic Games, Inc. All Rights Reserved.

#include "CombatGameCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "EnemyCharacter.h"
#include "Components/WidgetComponent.h"
#include "HealthBar.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimMontage.h"
#include "Scoree.h"
#include "Particles/ParticleSystemComponent.h"


DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ACombatGameCharacter

ACombatGameCharacter::ACombatGameCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	CameraBoom->bDoCollisionTest = false;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
	// 
	HealthWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	HealthWidgetComp->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	Health = 100;
	bReplicates = true;
	SetReplicateMovement(true);
	Score = 0;

	ScoreWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("Scoree"));
	ScoreWidgetComp->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	SuperPower = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("FireEffect"));
	SuperPower->SetupAttachment(GetMesh()); // Attach to the skeletal mesh
	SuperPower->bAutoActivate = false;      // Disable auto-activation

	Damage = 50;
}

void ACombatGameCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("mula"));
	UpdateHealthBar(Health, 100);
	/*SuperPower->Activate();*/
}

//////////////////////////////////////////////////////////////////////////
// Input

void ACombatGameCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACombatGameCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACombatGameCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}

	// Bind the attack input action to the PerformAttack function
	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &ACombatGameCharacter::PerformAttack);
	PlayerInputComponent->BindAction("Attack2", IE_Pressed, this, &ACombatGameCharacter::PerformAttack2);
}

void ACombatGameCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ACombatGameCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ACombatGameCharacter::PerformAttack()
{
	UE_LOG(LogTemp, Log, TEXT("serangggg"));
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AttackAnimMontage && AnimInstance)
	{
		// Play the attack animation
		//PlayAnimMontage(AttackAnimMontage);
		
		if (!AnimInstance->Montage_IsPlaying(AttackAnimMontage))
		{
			// Only the server should trigger this function
			if (HasAuthority()) // Ensure only the server plays the animation
			{
				// Call the multicast function to play the animation on all clients
				MulticastPlayAttackAnimation(AttackAnimMontage);
			}
			AnimInstance->Montage_Play(AttackAnimMontage);
			//mula
			// Define the reach distance for the attack (e.g., 200 units)
			float AttackRange = 200.0f;

			// Get the start and end points for the sphere trace (attack range)
			FVector StartLocation = GetActorLocation();
			FVector ForwardVector = GetActorForwardVector();
			FVector EndLocation = StartLocation + (ForwardVector * AttackRange);

			// Perform the sphere trace
			FHitResult HitResult;
			FCollisionQueryParams CollisionParams;
			CollisionParams.AddIgnoredActor(this); // Ignore the attacking character itself

			bool bHit = GetWorld()->SweepSingleByChannel(
				HitResult,
				StartLocation,
				EndLocation,
				FQuat::Identity, // No rotation for a simple line sweep
				ECC_Visibility,  // Use an appropriate collision channel
				FCollisionShape::MakeSphere(50.0f), // A radius representing the attack's reach
				CollisionParams
			);

			if (bHit)
			{
				UE_LOG(LogTemp, Log, TEXT("kena hit"));
				//enemy yang kena hit
				AActor* HitActor = HitResult.GetActor();
				AEnemyCharacter* HitEnemy = Cast<AEnemyCharacter>(HitResult.GetActor());
				 // Vector pointing from player to enemy
				if (HitActor && HitActor->IsA(AEnemyCharacter::StaticClass()) && HitEnemy)
				{
					UE_LOG(LogTemp, Warning, TEXT("Enemy Hit!"));
					if (HitEnemy != nullptr)
					{
						FVector HitDirection = (HitEnemy->GetActorLocation() - GetActorLocation()).GetSafeNormal();
						HitEnemy->Test(HitDirection,Damage);
						//float KnockbackStrength = 1000.0f;
						//// Apply the force to the HitEnemy to knock it back
						//UPrimitiveComponent* HitEnemyRoot = Cast<UPrimitiveComponent>(HitEnemy->GetRootComponent());
						//HitEnemyRoot->AddImpulse(HitDirection * KnockbackStrength, NAME_None, true);
						
					}

					

				}

			}
			//akhir

		}
	}
}
void ACombatGameCharacter::PerformAttack2()
{
	UE_LOG(LogTemp, Log, TEXT("serangggg attack no 2"));
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AttackAnimMontage2 && AnimInstance)
	{
		if (!AnimInstance->Montage_IsPlaying(AttackAnimMontage2))
		{
			if (HasAuthority()) // Ensure only the server plays the animation
			{
				// Call the multicast function to play the second animation on all clients
				MulticastPlayAttackAnimation(AttackAnimMontage2);
			}
			AnimInstance->Montage_Play(AttackAnimMontage2);
			//mula
			// Define the reach distance for the attack (e.g., 200 units)
			float AttackRange = 200.0f;
			// Get the start and end points for the sphere trace (attack range)
			FVector StartLocation = GetActorLocation();
			FVector ForwardVector = GetActorForwardVector();
			FVector EndLocation = StartLocation + (ForwardVector * AttackRange);
			// Perform the sphere trace
			FHitResult HitResult;
			FCollisionQueryParams CollisionParams;
			CollisionParams.AddIgnoredActor(this); // Ignore the attacking character itself
			bool bHit = GetWorld()->SweepSingleByChannel(
				HitResult,
				StartLocation,
				EndLocation,
				FQuat::Identity, // No rotation for a simple line sweep
				ECC_Visibility,  // Use an appropriate collision channel
				FCollisionShape::MakeSphere(50.0f), // A radius representing the attack's reach
				CollisionParams
			);

			if (bHit)
			{
				UE_LOG(LogTemp, Log, TEXT("kena hit"));
				//enemy yang kena hit
				AActor* HitActor = HitResult.GetActor();
				AEnemyCharacter* HitEnemy = Cast<AEnemyCharacter>(HitResult.GetActor());
				// Vector pointing from player to enemy
				if (HitActor && HitActor->IsA(AEnemyCharacter::StaticClass()) && HitEnemy)
				{
					UE_LOG(LogTemp, Warning, TEXT("Enemy Hit!"));
					if (HitEnemy != nullptr)
					{
						FVector HitDirection = (HitEnemy->GetActorLocation() - GetActorLocation()).GetSafeNormal();
						HitEnemy->Test(HitDirection,Damage);
					}

				}
			}
		}
	}
}

void ACombatGameCharacter::UpdateHealthBar(float CurrentHealth, float MaxHealth)
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

void ACombatGameCharacter::MulticastPlayAttackAnimation_Implementation(UAnimMontage* MontageToPlay)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && MontageToPlay)
	{
		if (!AnimInstance->Montage_IsPlaying(MontageToPlay))
		{
			AnimInstance->Montage_Play(MontageToPlay);
		}
	}
}

void ACombatGameCharacter::UpdateScore(int32 Points)
{
	Score += Points;
	UE_LOG(LogTemp, Log, TEXT("Score: %d"), Score);
	if (UUserWidget* Widget = ScoreWidgetComp->GetUserWidgetObject())
	{
		UScoree* Markah = Cast<UScoree>(Widget);
		if (Markah)
		{
			// Update the health bar widget
			Markah->UpdateScore(Score);
		}
	}
	if (Score == 40)
	{
		SuperPower->Activate();
		Damage = 100;
	}
}

void ACombatGameCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACombatGameCharacter, Score);
}
void ACombatGameCharacter::OnRep_Score()
{
	UE_LOG(LogTemp, Log, TEXT("Score: replicate masukk"));
	UpdateScore(20);
}