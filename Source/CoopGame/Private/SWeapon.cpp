// Fill out your copyright notice in the Description page of Project Settings.

#include "SWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "CoopGame.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"


static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing(TEXT("COOP.DebugWeapons"), 
	DebugWeaponDrawing,
	TEXT("Draw Debug Lines for Weapons"),
	ECVF_Cheat);

// Sets default values
ASWeapon::ASWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "Target";
	BaseDamage = 20.0;

	RateOfFire = 600;

	SetReplicates(true);

	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
}

void ASWeapon::BeginPlay() {
	Super::BeginPlay();

	TimeBetweenShots = 60 / RateOfFire;
}

void ASWeapon::Fire() {
	//Trace the world, from pawn eyes to crosshair location
	
	if (Role < ROLE_Authority) {
		ServerFire();
	}

	AActor* myOwner = GetOwner();
	if (myOwner) {
		FVector eyeLocation;
		FRotator eyeRotation;
		myOwner->GetActorEyesViewPoint(eyeLocation, eyeRotation);

		FVector shotDirection = eyeRotation.Vector();

		FVector traceEnd = eyeLocation + (eyeRotation.Vector() * 1000);

		FCollisionQueryParams queryParams;
		queryParams.AddIgnoredActor(myOwner);
		queryParams.AddIgnoredActor(this);
		queryParams.bTraceComplex = true;
		queryParams.bReturnPhysicalMaterial = true;

		//Particle "Target Parameter"
		FVector TracerEndPoint = traceEnd;

		EPhysicalSurface SurfaceType = SurfaceType_Default;

		FHitResult hit;
		if (GetWorld()->LineTraceSingleByChannel(hit, eyeLocation, traceEnd, COLLISION_WEAPON, queryParams)) {
			//blocking hit process damage

			AActor* hitActor = hit.GetActor();

			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(hit.PhysMaterial.Get());

			float ActualDamage = BaseDamage;
			if (SurfaceType == SURFACE_FLESHVULNERABLE)
			{
				ActualDamage = 40.0;
			}

			UGameplayStatics::ApplyPointDamage(hitActor, ActualDamage, shotDirection, hit, myOwner->GetInstigatorController(), this, damageType);

			PlayImpactEffects(SurfaceType, hit.ImpactPoint);

			if (DefaultImpactEffect) {
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DefaultImpactEffect, hit.ImpactPoint, hit.ImpactNormal.Rotation());
			}

			TracerEndPoint = hit.ImpactPoint;
		}

		if (DebugWeaponDrawing > 0) {
			DrawDebugLine(GetWorld(), eyeLocation, traceEnd, FColor::White, false, 1.0f, 0, 1.0f);
		}
		PlayFireEffects(TracerEndPoint);

		if (Role == ROLE_Authority)
		{
			HitScanTrace.TraceTo = TracerEndPoint;

			HitScanTrace.SurfaceType = SurfaceType;
		}

		LastFiredTime = GetWorld()->TimeSeconds;
	}
}

void ASWeapon::ServerFire_Implementation()
{
	Fire();
}

bool ASWeapon::ServerFire_Validate()
{
	return true;
}

void ASWeapon::OnRep_HitScanTrace()
{
	//play cosmetic fx
	PlayFireEffects(HitScanTrace.TraceTo);

	PlayImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.TraceTo);
}

void ASWeapon::StartFire()
{
	float FirstDelay = FMath::Max(LastFiredTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);
	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ASWeapon::Fire, TimeBetweenShots,true, FirstDelay);
}

void ASWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}

void ASWeapon::PlayFireEffects(FVector TracerEndPoint) {
	if (MuzzleEffect) {
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
	}


	if (TracerEffect) {
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);
		if (TracerComp) {
			TracerComp->SetVectorParameter(TracerTargetName, TracerEndPoint);
		}
	}

	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner) {
		APlayerController* PC= Cast<APlayerController>(MyOwner->GetController());
		if (PC) {
			PC->ClientPlayCameraShake(FireCamShake);
		}
	}
 

}

void ASWeapon::PlayImpactEffects(EPhysicalSurface SurfaceType, FVector Impactpoint)
{
	UParticleSystem* SelectedEffect = nullptr;

	switch (SurfaceType) {
	case SURFACE_FLESHDEFAULT:
		SelectedEffect = FleshImpactEffect;
		break;
	case SURFACE_FLESHVULNERABLE:
		SelectedEffect = FleshImpactEffect;
		break;
	default:
		SelectedEffect = DefaultImpactEffect;
		break;
	}

	if (SelectedEffect) {
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		FVector ShotDirection = Impactpoint - MuzzleLocation;
		ShotDirection.Normalize();
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, Impactpoint,ShotDirection.Rotation());
	}
}

void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner);
}


