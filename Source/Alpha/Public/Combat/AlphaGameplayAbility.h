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
	// 便捷获取角色拥有者
	UFUNCTION(BlueprintCallable, Category = "Ability")
	APlayerMaster* GetPlayerMaster() const;

	// 通用播 Montage 封装（连招、冲刺斩等都复用）
	UFUNCTION(BlueprintCallable, Category = "Ability")
	UAbilityTask_PlayMontageAndWait* PlayMontageTask(
		UAnimMontage* Montage, float Rate = 1.0f, FName Section = NAME_None);
};
