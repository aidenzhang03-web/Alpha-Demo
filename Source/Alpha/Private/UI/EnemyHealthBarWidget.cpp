#include "UI/EnemyHealthBarWidget.h"
#include "Components/ProgressBar.h"
#include "Combat/AlphaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"


void UEnemyHealthBarWidget::InitializeHealthBar(UAbilitySystemComponent* InASC)
{
	if (!InASC || bInitialized) return;

	const UAlphaAttributeSet* AttrSet = InASC->GetSet<UAlphaAttributeSet>();
	if (!AttrSet) return;   // ASC 未 InitAbilityActorInfo 时返回 null

	CachedASC = InASC;
	bInitialized = true;

	// 事件驱动：血量一变就刷新血条
	InASC->GetGameplayAttributeValueChangeDelegate(AttrSet->GetHealthAttribute())
		.AddUObject(this, &UEnemyHealthBarWidget::OnHealthChanged);

	RefreshBar();
}

void UEnemyHealthBarWidget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (!HealthBar) return;
	const UAlphaAttributeSet* AttrSet =
		CachedASC.IsValid() ? CachedASC->GetSet<UAlphaAttributeSet>() : nullptr;
	if (!AttrSet) return;

	const float Max = AttrSet->GetMaxHealth();
	HealthBar->SetPercent(Max > 0.f ? (Data.NewValue / Max) : 0.f);

	// 满血隐藏，受伤才显示
	SetVisibility(Data.NewValue >= Max ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
}


void UEnemyHealthBarWidget::RefreshBar()
{
	if (!HealthBar) return;
	const UAlphaAttributeSet* AttrSet =
		CachedASC.IsValid() ? CachedASC->GetSet<UAlphaAttributeSet>() : nullptr;
	if (!AttrSet) return;

	const float Max = AttrSet->GetMaxHealth();
	HealthBar->SetPercent(Max > 0.f ? (AttrSet->GetHealth() / Max) : 0.f);
}