// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AlphaGameplayAbility.generated.h"


class APlayerMaster;
class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;

/**
 * 
 */
UCLASS()
class ALPHA_API UAlphaGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	
public:

	// 统一激活检查：先走基类规则，再拦截死亡状态。
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	// 便捷获取角色拥有者
	UFUNCTION(BlueprintCallable, Category = "Ability")
	APlayerMaster* GetPlayerMaster() const;

	// 通用播 Montage 封装（连招、冲刺斩等都复用）
	UFUNCTION(BlueprintCallable, Category = "Ability")
	UAbilityTask_PlayMontageAndWait* PlayMontageTask(
		UAnimMontage* Montage, float Rate = 1.0f, FName Section = NAME_None);
};
