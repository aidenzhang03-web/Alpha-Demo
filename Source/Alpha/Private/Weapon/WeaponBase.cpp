// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/WeaponBase.h"

// Sets default values
AWeaponBase::AWeaponBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;  //需要时再启动

	WeaponStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponStaticMesh"));

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;


	// ===== 攻击判定盒 =====
	HitboxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Hitbox"));
	HitboxComponent->SetupAttachment(RootComponent);
	WeaponStaticMesh->SetupAttachment(RootComponent);

	// 默认关闭碰撞，只有攻击命中窗口内才开启
	WeaponStaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitboxComponent->SetCollisionObjectType(ECC_WorldDynamic);          // 可换成自定义 Weapon 通道
	HitboxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitboxComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // 只检测 Pawn（敌人）
	HitboxComponent->SetGenerateOverlapEvents(true);
	HitboxComponent->OnComponentBeginOverlap.AddDynamic(this, &AWeaponBase::OnHitboxBeginOverlap);


}

// Called when the game starts or when spawned
void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


// 开启攻击判定盒
void AWeaponBase::EnableHitbox()
{
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}


// 关闭攻击判定盒
void AWeaponBase::DisableHitbox()
{
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}


//碰撞盒 Overlap 回调
void AWeaponBase::OnHitboxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == GetOwner()) return;  // 忽略玩家自身
	OnWeaponHit.Broadcast(OtherActor);
}

