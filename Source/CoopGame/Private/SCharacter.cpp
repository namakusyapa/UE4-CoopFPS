// Fill out your copyright notice in the Description page of Project Settings.
#include "SCharacter.h"
#include "SWeapon.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SHealthComponent.h"
#include "CoopGame.h"

// Sets default values
ASCharacter::ASCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	springArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	springArmComp->bUsePawnControlRotation = true;
	springArmComp->SetupAttachment(RootComponent);

	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;


	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));

	cameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	cameraComp->SetupAttachment(springArmComp);

	ZoomedFOV = 65.0f;
	ZoomInterpSpeed = 20;
	WeaponAttachSocketName = "WeaponSocket";

	bDied = false;
}

// Called when the game starts or when spawned
void ASCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	DefaultFOV = cameraComp->FieldOfView;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	CurrentWeapon = GetWorld()->SpawnActor<ASWeapon>(StarterWeaponClass, FVector::ZeroVector, 
		FRotator::ZeroRotator, SpawnParams);
	if (CurrentWeapon) {
		CurrentWeapon->SetOwner(this);
		CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			WeaponAttachSocketName);
	}

	HealthComp->OnHealthChanged.AddDynamic(this, &ASCharacter::OnHealthChanged);
}

void ASCharacter::MoveForward(float value) {
	AddMovementInput(GetActorForwardVector() * value);
}

void ASCharacter::MoveRight(float value) {
	AddMovementInput(GetActorRightVector() * value);
}

void ASCharacter::BeginCrouch() {
	Crouch();
}

void ASCharacter::EndCrouch() {
	UnCrouch();
}

void ASCharacter::BeginZoom() {
	bWantsToZoom = true;
}

void ASCharacter::EndZoom() {
	bWantsToZoom = false;
}

void ASCharacter::StartFire() {
	if (CurrentWeapon) {
		CurrentWeapon->StartFire();
	}
}


void ASCharacter::StopFire() {
	if (CurrentWeapon) {
		CurrentWeapon->StopFire();
	}
}

void ASCharacter::OnHealthChanged(USHealthComponent* HealthComp, float Health, float HealthDelta,
	const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Health <= 0.0f &&!bDied) {
		//Die
		bDied = true;
		GetMovementComponent()->StopMovementImmediately();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		DetachFromControllerPendingDestroy();

		SetLifeSpan(10.0f);
	}
}

// Called every frame
void ASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	float TargetFOV = bWantsToZoom ? ZoomedFOV : DefaultFOV;

	float NewFOV = FMath::FInterpTo(cameraComp->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed);
	cameraComp->SetFieldOfView(NewFOV);
}

// Called to bind functionality to input
void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacter::MoveRight);

	PlayerInputComponent->BindAxis("LookUp", this, &ASCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &ASCharacter::AddControllerYawInput);
	
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASCharacter::EndCrouch);

	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &ASCharacter::BeginZoom);
	PlayerInputComponent->BindAction("Zoom", IE_Released, this, &ASCharacter::EndZoom);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASCharacter::StopFire);

}

FVector ASCharacter::GetPawnViewLocation()const {
	if (cameraComp){
		return cameraComp->GetComponentLocation();
	}
	return Super::GetPawnViewLocation();
}

