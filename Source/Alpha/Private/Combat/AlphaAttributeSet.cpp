// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/AlphaAttributeSet.h"
#include "GameplayEffectExtension.h"

UAlphaAttributeSet::UAlphaAttributeSet()
{
    //默认值：这里用 Init* 设 BaseValue
    InitHealth(100.f);
    InitMaxHealth(100.f);
    InitMana(50.f);
    InitMaxMana(50.f);
    InitStamina(100.f);
    InitMaxStamina(100.f);
    
}

void UAlphaAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    // 只钳制本次被修改的那个属性，避免无谓钳制
    const FGameplayAttribute ModifiedAttribute = Data.EvaluatedData.Attribute;

    if (ModifiedAttribute == GetHealthAttribute())
    {
        SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));

        // 生命归零 → 死亡处理（这里留占位，建议发 GameplayTag 事件，而非直接 Destroy）
        if (GetHealth() <= 0.f)
        {
            // TODO: 广播死亡事件
        }
    }
    else if (ModifiedAttribute == GetManaAttribute())
    {
        SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
    }
    else if (ModifiedAttribute == GetStaminaAttribute())
    {
        SetStamina(FMath::Clamp(GetStamina(), 0.f, GetMaxStamina()));
    }
    
}