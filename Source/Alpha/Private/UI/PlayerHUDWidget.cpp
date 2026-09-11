#include "UI/PlayerHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Player/PlayerMaster.h"
#include "Combat/AlphaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"

void UPlayerHUDWidget::InitializeHUD(APlayerMaster* InPlayer)
{
    if (!InPlayer || bInitialized) return;

    UAbilitySystemComponent* ASC = InPlayer->GetAbilitySystemComponent();
    if (!ASC) return;

    const UAlphaAttributeSet* AttrSet = ASC->GetSet<UAlphaAttributeSet>();
    if (!AttrSet) return;   // 若这里返回 null，说明 ASC 没 InitAbilityActorInfo

    OwningPlayer = InPlayer;
    CachedASC = ASC;
    bInitialized = true;

    // 事件驱动：属性一变，回调里刷新对应条（符合项目约定）
    ASC->GetGameplayAttributeValueChangeDelegate(AttrSet->GetHealthAttribute())
        .AddUObject(this, &UPlayerHUDWidget::OnHealthChanged);
    ASC->GetGameplayAttributeValueChangeDelegate(AttrSet->GetManaAttribute())
        .AddUObject(this, &UPlayerHUDWidget::OnManaChanged);
    ASC->GetGameplayAttributeValueChangeDelegate(AttrSet->GetStaminaAttribute())
        .AddUObject(this, &UPlayerHUDWidget::OnStaminaChanged);

    RefreshAllBars();
}

// 生命值
void UPlayerHUDWidget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
    if (!HealthBar || !OwningPlayer.IsValid()) return;
    const float Max = OwningPlayer->GetMaxHealth();
    HealthBar->SetPercent(Max > 0.f ? (Data.NewValue / Max) : 0.f);
}

// 法力值
void UPlayerHUDWidget::OnManaChanged(const FOnAttributeChangeData& Data)
{
    if (!ManaBar || !OwningPlayer.IsValid()) return;
    const float Max = OwningPlayer->GetMaxMana();
    ManaBar->SetPercent(Max > 0.f ? (Data.NewValue / Max) : 0.f);
}

// 耐力值
void UPlayerHUDWidget::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
    if (!StaminaBar || !OwningPlayer.IsValid()) return;
    const float Max = OwningPlayer->GetMaxStamina();
    StaminaBar->SetPercent(Max > 0.f ? (Data.NewValue / Max) : 0.f);
}

// 进度条刷新
void UPlayerHUDWidget::RefreshAllBars()
{
    if (!OwningPlayer.IsValid()) return;

    if (HealthBar)
    {
        const float Max = OwningPlayer->GetMaxHealth();
        HealthBar->SetPercent(Max > 0.f ? (OwningPlayer->GetHealth() / Max) : 0.f);
    }
    if (ManaBar)
    {
        const float Max = OwningPlayer->GetMaxMana();
        ManaBar->SetPercent(Max > 0.f ? (OwningPlayer->GetMana() / Max) : 0.f);
    }
    if (StaminaBar)
    {
        const float Max = OwningPlayer->GetMaxStamina();
        StaminaBar->SetPercent(Max > 0.f ? (OwningPlayer->GetStamina() / Max) : 0.f);
    }
}