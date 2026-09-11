// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapon/WeaponBase.h"
#include "WeaponComponent.generated.h"

class UAnimMontage;
class ACharacter;

// 单把武器的配置（在角色蓝图的 WeaponConfigs 数组里为每把武器配一份）
USTRUCT(BlueprintType)
struct FWeaponConfig
{
	GENERATED_BODY()

	// 武器蓝图类（AWeaponBase 子类，如 BP_Sword / BP_Spear / BP_Greatsword）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AWeaponBase> WeaponClass;

	// 背部插槽（收起时挂的位置）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName BackSlotName = "weapon_back_l_slot";

	// 手部插槽（拔出时挂的位置）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName HandSlotName = "weapon_r_slot";

	// 拔出武器动画（Montage 关键帧由 AnimNotify 触发 AttachWeaponToHand）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<UAnimMontage> DrawWeaponMontage;

	// 收起武器动画（Montage 关键帧由 AnimNotify 触发 AttachWeaponToBack）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<UAnimMontage> SheatheWeaponMontage;

	// 单次命中的基础伤害
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float BaseDamage = 20.f;
};



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALPHA_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWeaponComponent();


	// 生成并初始附着到背部的武器
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SpawnAndAttachWeapon();

	// 切换武器到右手插槽
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void AttachWeaponToHand();

	// 切换武器回背部
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void AttachWeaponToBack();

	// 收拔切换（点按）：拔出/收起。内部处理 Montage 播放与防连点锁。
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ToggleWeapon();

	// 切换到指定索引的武器（核心逻辑：销毁旧武器，生成新武器）
	// 参数 NewIndex：目标武器在 WeaponConfigs 数组中的下标。
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SwitchWeapon(int32 NewIndex);

	// 循环切换到下一把武器（供 E 键输入使用）。
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SwitchWeaponNext();

	// 循环切换到上一把武器（供 Q 键输入使用）。
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SwitchWeaponPrev();

	// 获取当前武器的配置（越界返回 nullptr）
	const FWeaponConfig* GetCurrentWeaponConfig() const;

	// 状态查询（供动画层 / 输入组件 / UI 使用）
	bool IsWeaponDrawn() const { return bWeaponIsDrawn; }
	bool IsWeaponTransitionPlaying() const { return bWeaponTransitionPlaying; }
	int32 GetWeaponCount() const { return WeaponConfigs.Num(); }
	int32 GetCurrentWeaponIndex() const { return CurrentWeaponIndex; }

	// 开启攻击命中窗口（清空已命中集合，打开武器碰撞盒）
	void EnableWeaponHitbox();

	// 关闭攻击命中窗口（关闭武器碰撞盒）
	void DisableWeaponHitbox();

	// 武器命中回调（由 AWeaponBase::OnWeaponHit 委托触发）
	UFUNCTION()
	void OnWeaponHit(AActor* HitActor);

private:

	// 本次命中窗口内已命中的目标（防同一目标被多次命中）
	TSet<TObjectPtr<AActor>> HitActorsThisWindow;




protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// 播放收拔动画并上锁（防连点）。
	void PlayWeaponMontage(UAnimMontage* Montage);

	// 获取角色拥有者（用于 GetMesh 挂插槽 / PlayAnimMontage）。
	ACharacter* GetOwnerCharacter() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TArray<FWeaponConfig> WeaponConfigs;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<AWeaponBase> EquippedWeapon;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	int32 CurrentWeaponIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool bWeaponIsDrawn = false;

	// 武器收拔动画是否正在播放（防连点）
	bool bWeaponTransitionPlaying = false;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
