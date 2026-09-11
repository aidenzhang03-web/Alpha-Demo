// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/WeaponComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Combat/AlphaAttributeComponent.h"

// Sets default values for this component's properties
UWeaponComponent::UWeaponComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	// 武器系统不需要每帧 Tick
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


ACharacter* UWeaponComponent::GetOwnerCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

// 播放收拔武器动画
void UWeaponComponent::PlayWeaponMontage(UAnimMontage* Montage)
{
	if (!Montage) return;

	if (ACharacter* OwnerChar = GetOwnerCharacter())
	{
		bWeaponTransitionPlaying = true;   // 播放期间上锁，防止连点重复播放
		OwnerChar->PlayAnimMontage(Montage);
	}
}

//将武器生成在背上
void UWeaponComponent::SpawnAndAttachWeapon()
{
	if (EquippedWeapon) return;   // 已有武器则跳过

	const FWeaponConfig* Config = GetCurrentWeaponConfig();
	if (!Config || !Config->WeaponClass) return;

	ACharacter* OwnerChar = GetOwnerCharacter();
	if (!OwnerChar) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerChar;
	SpawnParams.Instigator = OwnerChar->GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWeaponBase* NewWeapon = GetWorld()->SpawnActor<AWeaponBase>(
		Config->WeaponClass, FTransform::Identity, SpawnParams);
	if (!NewWeapon) return;

	// 附着到背部插槽
	USkeletalMeshComponent* MeshComp = OwnerChar->GetMesh();
	if (MeshComp && Config->BackSlotName != NAME_None)
	{
		NewWeapon->AttachToComponent(
			MeshComp,
			FAttachmentTransformRules::SnapToTargetIncludingScale,
			Config->BackSlotName);
	}

	// 绑定武器命中委托（SwitchWeapon 会销毁重建武器，绑定放这里保证每次都绑上）
		NewWeapon->OnWeaponHit.AddDynamic(this, &UWeaponComponent::OnWeaponHit);

	EquippedWeapon = NewWeapon;
}

//拔出武器
void UWeaponComponent::AttachWeaponToHand()
{
	if (!EquippedWeapon) return;

	const FWeaponConfig* Config = GetCurrentWeaponConfig();
	ACharacter* OwnerChar = GetOwnerCharacter();
	if (!OwnerChar) return;

	USkeletalMeshComponent* MeshComp = OwnerChar->GetMesh();
	if (!MeshComp || !Config || Config->HandSlotName == NAME_None) return;

	EquippedWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	EquippedWeapon->AttachToComponent(
		MeshComp,
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		Config->HandSlotName);

	bWeaponIsDrawn = true;
	bWeaponTransitionPlaying = false;   // 关键帧已到，切换动画完成，解锁
}

//收起武器
void UWeaponComponent::AttachWeaponToBack()
{
	if (!EquippedWeapon) return;

	const FWeaponConfig* Config = GetCurrentWeaponConfig();
	ACharacter* OwnerChar = GetOwnerCharacter();
	if (!OwnerChar) return;

	USkeletalMeshComponent* MeshComp = OwnerChar->GetMesh();
	if (!MeshComp || !Config || Config->BackSlotName == NAME_None) return;

	EquippedWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	EquippedWeapon->AttachToComponent(
		MeshComp,
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		Config->BackSlotName);

	bWeaponIsDrawn = false;
	bWeaponTransitionPlaying = false;
}

// 获取当前武器的配置（越界返回 nullptr）
const FWeaponConfig* UWeaponComponent::GetCurrentWeaponConfig() const
{
	if (!WeaponConfigs.IsValidIndex(CurrentWeaponIndex)) return nullptr;
	return &WeaponConfigs[CurrentWeaponIndex];
}

// 切换到指定索引的武器（销毁旧武器，生成新武器）
void UWeaponComponent::SwitchWeapon(int32 NewIndex)
{
	if (bWeaponTransitionPlaying) return;               // 收拔动画播放中，忽略
	if (NewIndex == CurrentWeaponIndex) return;         // 同一把，忽略
	if (!WeaponConfigs.IsValidIndex(NewIndex)) return;  // 越界保护
	if (!WeaponConfigs[NewIndex].WeaponClass) return;   // 目标未配置类

	// 若当前武器已拔出，先直接收回（不播动画，简化处理）
	if (bWeaponIsDrawn && EquippedWeapon)
	{
		AttachWeaponToBack();
	}

	// 销毁旧武器
	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;
	}

	// 更新索引并生成新武器（自动挂到背部插槽）
	CurrentWeaponIndex = NewIndex;
	bWeaponIsDrawn = false;
	SpawnAndAttachWeapon();
}

// 切换下一把武器
void UWeaponComponent::SwitchWeaponNext()
{
	if (WeaponConfigs.Num() <= 1) return;
	const int32 NextIndex = (CurrentWeaponIndex + 1) % WeaponConfigs.Num();
	SwitchWeapon(NextIndex);
}

// 切换上一把武器
void UWeaponComponent::SwitchWeaponPrev()
{
	if (WeaponConfigs.Num() <= 1) return;
	const int32 PrevIndex =
		(CurrentWeaponIndex - 1 + WeaponConfigs.Num()) % WeaponConfigs.Num();
	SwitchWeapon(PrevIndex);
}

// 拔出/收起武器
void UWeaponComponent::ToggleWeapon()
{
	if (bWeaponTransitionPlaying) return;   // 动画播放中防连点

	const FWeaponConfig* Config = GetCurrentWeaponConfig();
	if (!Config) return;

	if (bWeaponIsDrawn)
	{
		if (Config->SheatheWeaponMontage)
		{
			PlayWeaponMontage(Config->SheatheWeaponMontage);
		}
		else
		{
			AttachWeaponToBack();   // 未配置 montage 时兜底：直接收起
		}
	}
	else
	{
		if (Config->DrawWeaponMontage)
		{
			PlayWeaponMontage(Config->DrawWeaponMontage);
		}
		else
		{
			AttachWeaponToHand();   // 未配置 montage 时兜底：直接拔出
		}
	}
}

// 开启攻击命中窗口
void UWeaponComponent::EnableWeaponHitbox()
{
	HitActorsThisWindow.Empty();                 // 新窗口清空，允许再次命中
	if (EquippedWeapon) EquippedWeapon->EnableHitbox();
}

// 关闭攻击命中窗口
void UWeaponComponent::DisableWeaponHitbox()
{
	if (EquippedWeapon) EquippedWeapon->DisableHitbox();
	HitActorsThisWindow.Empty();
}

// 武器命中回调
void UWeaponComponent::OnWeaponHit(AActor* HitActor)
{
	if (!HitActor) return;
	if (HitActorsThisWindow.Contains(HitActor)) return;  // 同一目标本次挥砍只命中一次
	HitActorsThisWindow.Add(HitActor);

	AActor* Owner = GetOwner();
	if (!Owner) return;

	const FWeaponConfig* Config = GetCurrentWeaponConfig();
	if (!Config) return;

	// 通过 Owner 上的属性组件应用伤害（玩家/敌人通用）
	if (UAlphaAttributeComponent* AttrComp = Owner->FindComponentByClass<UAlphaAttributeComponent>())
	{
		AttrComp->ApplyDamageToTarget(HitActor, Config->BaseDamage);
	}
}