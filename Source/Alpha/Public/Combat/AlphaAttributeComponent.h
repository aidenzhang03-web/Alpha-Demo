#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ActiveGameplayEffectHandle.h"
#include "AlphaAttributeComponent.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;



// 属性效果集中配置（蓝图里一处配完所有属性 GE）
USTRUCT(BlueprintType)
struct FAttributeEffectsConfig
{
    GENERATED_BODY()

    // 扣血（Instant + SetByCaller 传伤害值）
    UPROPERTY(EditDefaultsOnly, Category = "Attributes")
    TSubclassOf<UGameplayEffect> HealthCostEffect;

    // 耗法力（Instant + SetByCaller 传消耗值）
    UPROPERTY(EditDefaultsOnly, Category = "Attributes")
    TSubclassOf<UGameplayEffect> ManaCostEffect;

    // 耗耐力（Instant + SetByCaller 传消耗值）
    UPROPERTY(EditDefaultsOnly, Category = "Attributes")
    TSubclassOf<UGameplayEffect> StaminaCostEffect;

    // 耐力自动回复（Infinite + Period，固定值）
    UPROPERTY(EditDefaultsOnly, Category = "Attributes")
    TSubclassOf<UGameplayEffect> StaminaRegenEffect;

    // 持续扣耐力（通用：冲刺/蓄力/中毒等，Infinite + Period + SetByCaller）
    UPROPERTY(EditDefaultsOnly, Category = "Attributes")
    TSubclassOf<UGameplayEffect> StaminaDrainEffect;
};


/**
 * 角色属性组件：属性「写」的统一入口。
 * 所有属性修改都通过这里应用 GameplayEffect，不直接改 AttributeSet。
 * 属性「读」由角色类的 getter 提供。
 */

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALPHA_API UAlphaAttributeComponent : public UActorComponent
{
    GENERATED_BODY()


public:
    UAlphaAttributeComponent();


    // 通用 GE 应用入口：构造 Spec → 可选 SetByCaller → 应用到自身。
    // SetByCallerTag 无效时不传动态值（用 GE 里配好的固定值）。
    UFUNCTION(BlueprintCallable, Category = "Attributes")
    FActiveGameplayEffectHandle ApplyEffectToSelf(
        TSubclassOf<UGameplayEffect> EffectClass,
        FGameplayTag SetByCallerTag = FGameplayTag(),
        float SetByCallerValue = 0.f);

    // 通用 GE 应用入口：把效果应用到「目标」Actor（目标需实现 IAbilitySystemInterface）。
    // SetByCallerTag 无效时不传动态值（用 GE 里配好的固定值）。
    UFUNCTION(BlueprintCallable, Category = "Attributes")
    FActiveGameplayEffectHandle ApplyEffectToTarget(
        AActor* Target,
        TSubclassOf<UGameplayEffect> EffectClass,
        FGameplayTag SetByCallerTag = FGameplayTag(),
        float SetByCallerValue = 0.f);



    // ======== 语义封装（内部都走 ApplyEffectToSelf） ========
 
    // 扣血（传正数）
    UFUNCTION(BlueprintCallable, Category = "Attributes")
    void ApplyHealthCost(float Amount);

    // 对目标造成伤害（传正数）
    UFUNCTION(BlueprintCallable, Category = "Attributes")
    void ApplyDamageToTarget(AActor* Target, float Amount);

    // 耗法力（传正数）
    UFUNCTION(BlueprintCallable, Category = "Attributes")
    void ApplyManaCost(float Amount);     

    // 耗耐力（传正数）
    UFUNCTION(BlueprintCallable, Category = "Attributes")
    void ApplyStaminaCost(float Amount);  

    // 通用：开始持续扣耐力。DrainPerSecond = 每秒扣多少（正数）。
    UFUNCTION(BlueprintCallable, Category = "Attributes")
    void StartStaminaDrain(float DrainPerSecond);

    // 通用：停止持续扣耐力。
    UFUNCTION(BlueprintCallable, Category = "Attributes")
    void StopStaminaDrain();

    // 同步冲刺状态（内部用配置好的冲刺速率调 Start/Stop）
    UFUNCTION(BlueprintCallable, Category = "Attributes")
    void SetSprinting(bool bSprinting);




protected:

    virtual void BeginPlay() override;

    // 从拥有者（实现 IAbilitySystemInterface）拿 ASC
    UAbilitySystemComponent* GetASC() const;

private:

    // 缓存的 ASC（BeginPlay 时通过 GetOwner 获取一次）
    UPROPERTY()
    TObjectPtr<UAbilitySystemComponent> CachedASC;

    // GE 集中配置（蓝图里一处配完）
    UPROPERTY(EditDefaultsOnly, Category = "Attributes")
    FAttributeEffectsConfig AttributeEffects;


    // 冲刺消耗速率（每秒扣多少耐力）
    UPROPERTY(EditDefaultsOnly, Category = "Attributes|Sprint", meta = (ClampMin = "0"))
    float SprintDrainRate = 200.f;

    // 退出扣耐后延迟多久开始恢复（秒）
    UPROPERTY(EditDefaultsOnly, Category = "Attributes", meta = (ClampMin = "0"))
    float StaminaRegenDelay = 1.0f;


    // 耐力回复的活跃 handle（Infinite GE，需要保存以便将来移除）
    FActiveGameplayEffectHandle StaminaRegenHandle;

    // 持续扣耐的活跃 handle
    FActiveGameplayEffectHandle StaminaDrainHandle;

    // 扣耐是否激活（幂等）
    bool bStaminaDrainActive = false;

    // 延迟恢复定时器
    FTimerHandle StaminaRegenTimer;

    // 恢复/延迟辅助
    void StartStaminaRegenDelay();
    void ApplyStaminaRegen();
    void StopStaminaRegen();
};