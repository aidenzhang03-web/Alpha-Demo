// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AlphaAttributeSet.generated.h"

/**
 * 玩家基础属性集：生命值 / 法力值 / 耐力值（各含最大值）。
 * 属性数据存储在这里，但「修改」必须通过 GameplayEffect，不要直接调用 Setter。
 */

UCLASS()
class ALPHA_API UAlphaAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UAlphaAttributeSet();

    // GE 生效后的统一回调：在此处对属性做钳制（0 ~ Max），并处理死亡等派生逻辑
    virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;


    // ========== 生命值 ==========
        UPROPERTY(BlueprintReadOnly, Category = "Attributes|Vital")
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS_BASIC(UAlphaAttributeSet, Health);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes|Vital")
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS_BASIC(UAlphaAttributeSet, MaxHealth);

    // ========== 法力值 ==========
    UPROPERTY(BlueprintReadOnly, Category = "Attributes|Vital")
    FGameplayAttributeData Mana;
    ATTRIBUTE_ACCESSORS_BASIC(UAlphaAttributeSet, Mana);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes|Vital")
    FGameplayAttributeData MaxMana;
    ATTRIBUTE_ACCESSORS_BASIC(UAlphaAttributeSet, MaxMana);

    // ========== 耐力值 ==========
    UPROPERTY(BlueprintReadOnly, Category = "Attributes|Vital")
    FGameplayAttributeData Stamina;
    ATTRIBUTE_ACCESSORS_BASIC(UAlphaAttributeSet, Stamina);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes|Vital")
    FGameplayAttributeData MaxStamina;
    ATTRIBUTE_ACCESSORS_BASIC(UAlphaAttributeSet, MaxStamina);
};
