#include "Combat/AlphaAttributeComponent.h"
#include "Combat/AlphaAttributeSet.h"
#include "Combat/AlphaGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "TimerManager.h"


UAlphaAttributeComponent::UAlphaAttributeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;  // 事件驱动，无需 Tick
}



void UAlphaAttributeComponent::BeginPlay()
{
    Super::BeginPlay();

    // 缓存 ASC（通过 GAS 接口获取 ASC）
    CachedASC = GetASC();

}


UAbilitySystemComponent* UAlphaAttributeComponent::GetASC() const
{
    // 用通用 GAS 接口获取 ASC（玩家/敌人通用），不再依赖 APlayerMaster
    if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
    {
        return ASI->GetAbilitySystemComponent();
    }
    return nullptr;
}

// 对自身应用GE
FActiveGameplayEffectHandle UAlphaAttributeComponent::ApplyEffectToSelf(
    TSubclassOf<UGameplayEffect> EffectClass,
    FGameplayTag SetByCallerTag,
    float SetByCallerValue)
{
    if (!CachedASC || !EffectClass) return FActiveGameplayEffectHandle();

    FGameplayEffectContextHandle Context = CachedASC->MakeEffectContext();
    Context.AddSourceObject(GetOwner());

    FGameplayEffectSpecHandle Spec = CachedASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
    if (!Spec.IsValid()) return FActiveGameplayEffectHandle();

    // 仅在需要动态值时设置 SetByCaller（固定值效果跳过）
    if (SetByCallerTag.IsValid())
    {
        Spec.Data->SetSetByCallerMagnitude(SetByCallerTag, SetByCallerValue);
    }

    return CachedASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}


// 对目标应用 GE（目标需实现 IAbilitySystemInterface 且有自己的 ASC）
FActiveGameplayEffectHandle UAlphaAttributeComponent::ApplyEffectToTarget(
    AActor* Target, TSubclassOf<UGameplayEffect> EffectClass,
    FGameplayTag SetByCallerTag, float SetByCallerValue)
{
    if (!CachedASC || !EffectClass || !Target) return FActiveGameplayEffectHandle();

    // 目标必须实现 GAS 接口，且有自己的 ASC
    IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(Target);
    if (!TargetInterface) return FActiveGameplayEffectHandle();
    UAbilitySystemComponent* TargetASC = TargetInterface->GetAbilitySystemComponent();
    if (!TargetASC) return FActiveGameplayEffectHandle();

    // 用玩家自己的 ASC 构造 Spec，GE 资产引用来自攻击方（玩家），敌人无需配 GE
    FGameplayEffectContextHandle Context = CachedASC->MakeEffectContext();
    Context.AddSourceObject(GetOwner());
    Context.AddInstigator(GetOwner(), GetOwner());

    FGameplayEffectSpecHandle Spec = CachedASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
    if (!Spec.IsValid()) return FActiveGameplayEffectHandle();

    if (SetByCallerTag.IsValid())
    {
        Spec.Data->SetSetByCallerMagnitude(SetByCallerTag, SetByCallerValue);
    }

    return CachedASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
}


void UAlphaAttributeComponent::ApplyHealthCost(float Amount)
{
    ApplyEffectToSelf(AttributeEffects.HealthCostEffect, AlphaGameplayTags::Attribute_HealthCost, -Amount);
}

// 对目标造成伤害（传正数，内部转负值扣血）
void UAlphaAttributeComponent::ApplyDamageToTarget(AActor* Target, float Amount)
{
    ApplyEffectToTarget(Target, AttributeEffects.HealthCostEffect,
        AlphaGameplayTags::Attribute_HealthCost, -Amount);
}

void UAlphaAttributeComponent::ApplyManaCost(float Amount)
{
    ApplyEffectToSelf(AttributeEffects.ManaCostEffect, AlphaGameplayTags::Attribute_ManaCost, -Amount);
}

void UAlphaAttributeComponent::ApplyStaminaCost(float Amount)
{
    ApplyEffectToSelf(AttributeEffects.StaminaCostEffect, AlphaGameplayTags::Attribute_StaminaCost, -Amount);
}


// 通用：开始持续扣耐力
void UAlphaAttributeComponent::StartStaminaDrain(float DrainPerSecond)
{
    if (!CachedASC || !AttributeEffects.StaminaDrainEffect) return;
    if (bStaminaDrainActive) return;   // 幂等

    StopStaminaRegen();                 // 扣耐期间停恢复

    // 自动从 GE 资产读取 Period，消除手动维护 StaminaDrainPeriod 的隐患
    const UGameplayEffect* DrainGE = AttributeEffects.StaminaDrainEffect->GetDefaultObject<UGameplayEffect>();
    const float Period = DrainGE ? DrainGE->Period.GetValueAtLevel(1.f) : 0.f;

    // Period 无效（<=0）时退避，避免「每周期扣 0」导致扣耐静默失效
    if (Period <= KINDA_SMALL_NUMBER)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AlphaAttributeComponent] StaminaDrainEffect 的 Period 无效（<=0），请检查 GE 资产配置"));
        return;
    }

    // 每周期扣量 = 每秒扣量 × Period
    const float DrainPerTick = DrainPerSecond * Period;

    // SetByCaller 动态传每周期扣量（负值 = 扣）
    StaminaDrainHandle = ApplyEffectToSelf(
        AttributeEffects.StaminaDrainEffect,
        AlphaGameplayTags::Attribute_StaminaDrain,
        -DrainPerTick);

    bStaminaDrainActive = StaminaDrainHandle.IsValid();
}


// 通用：停止持续扣耐力
void UAlphaAttributeComponent::StopStaminaDrain()
{
    if (!bStaminaDrainActive) return;   // 幂等

    if (StaminaDrainHandle.IsValid())
    {
        CachedASC->RemoveActiveGameplayEffect(StaminaDrainHandle);
        StaminaDrainHandle.Invalidate();
    }
    bStaminaDrainActive = false;

    StartStaminaRegenDelay();           // 停止后延迟恢复
}


// 同步冲刺状态
void UAlphaAttributeComponent::SetSprinting(bool bSprinting)
{
    if (bSprinting)
        StartStaminaDrain(SprintDrainRate);   // 用配置好的冲刺速率
    else
        StopStaminaDrain();
}


// 延迟恢复
void UAlphaAttributeComponent::StartStaminaRegenDelay()
{
    if (StaminaRegenTimer.IsValid())
        GetWorld()->GetTimerManager().ClearTimer(StaminaRegenTimer);

    GetWorld()->GetTimerManager().SetTimer(
        StaminaRegenTimer,
        this,
        &UAlphaAttributeComponent::ApplyStaminaRegen,
        StaminaRegenDelay,
        false);
}


// 应用恢复 GE（延迟后触发）
void UAlphaAttributeComponent::ApplyStaminaRegen()
{
    if (!CachedASC || !AttributeEffects.StaminaRegenEffect) return;

    if (StaminaRegenHandle.IsValid())
        CachedASC->RemoveActiveGameplayEffect(StaminaRegenHandle);

    StaminaRegenHandle = ApplyEffectToSelf(AttributeEffects.StaminaRegenEffect);
}


// 停止恢复
void UAlphaAttributeComponent::StopStaminaRegen()
{
    if (StaminaRegenTimer.IsValid())
        GetWorld()->GetTimerManager().ClearTimer(StaminaRegenTimer);

    if (StaminaRegenHandle.IsValid())
    {
        CachedASC->RemoveActiveGameplayEffect(StaminaRegenHandle);
        StaminaRegenHandle.Invalidate();
    }
}