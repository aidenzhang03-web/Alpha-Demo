// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/AlphaAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

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


void UAlphaAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // COND_None            → 复制给所有客户端（血条 / HUD 需要，不能用 OwnerOnly）
    // REPNOTIFY_Always     → 值即使相同也触发 OnRep，避免「钳制回原值」时 UI 不同步
    DOREPLIFETIME_CONDITION_NOTIFY(UAlphaAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAlphaAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAlphaAttributeSet, Mana, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAlphaAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAlphaAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAlphaAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
}


void UAlphaAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAlphaAttributeSet, Health, OldHealth);
}


void UAlphaAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAlphaAttributeSet, MaxHealth, OldMaxHealth);
}


void UAlphaAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAlphaAttributeSet, Mana, OldMana);
}


void UAlphaAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAlphaAttributeSet, MaxMana, OldMaxMana);
}


void UAlphaAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAlphaAttributeSet, Stamina, OldStamina);
}


void UAlphaAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAlphaAttributeSet, MaxStamina, OldMaxStamina);
}
