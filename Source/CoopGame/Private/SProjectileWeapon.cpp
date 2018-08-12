// Fill out your copyright notice in the Description page of Project Settings.

#include "SProjectileWeapon.h"



void ASProjectileWeapon::Fire() {
	AActor* myOwner = GetOwner();
	if (myOwner && ProjectileClass) {
		FVector eyeLocation;
		FRotator eyeRotation;
		myOwner->GetActorEyesViewPoint(eyeLocation, eyeRotation);

		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, eyeRotation, SpawnParams);
	}
}
