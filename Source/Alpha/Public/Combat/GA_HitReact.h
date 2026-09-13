#pragma once

#include "CoreMinimal.h"
#include "Combat/AlphaGameplayAbility.h"
#include "GA_HitReact.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;


/**
 * 敌人受击能力（Hit React）：被命中时播放受击动画，并短暂锁定移动。
 *
 * 触发方式：由 AEnemyCharacter::OnHealthChanged 发送 Event.HitReact
 *           GameplayEvent，本能力通过 AbilityTriggers 监听该事件自动激活。
 *
 * 打断语义：
 *   - CancelAbilitiesWithTag = Ability.EnemyAttack：受击打断正在进行的攻击；
 *   - ActivationOwnedTags = State.HitReact：受击期间挂状态标记；
 *   - ActivationBlockedTags = State.HitReact：受击期间不可再激活（防抖）。
 *
 * 移动锁：激活时停 AI 移动并禁用 CharacterMovement，结束时恢复（见 .cpp）。
 */


UCLASS()
class ALPHA_API UGA_HitReact : public UAlphaGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_HitReact();

	/**
	 * 激活能力：停移动 → 播放受击 Montage → 结束回调里恢复。
	 * @param Handle            能力实例句柄
	 * @param ActorInfo         能力所属 Actor 信息（Avatar = AEnemyCharacter）
	 * @param ActivationInfo    激活信息
	 * @param TriggerEventData  触发事件负载（Event.HitReact，本能力不用其字段，预留方向性受击）
	 */

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:

	/// 受击动画 Montage（在 BP_GA_HitReact 的 Class Defaults 里指定）
	UPROPERTY(EditDefaultsOnly, Category = "HitReact")
	TObjectPtr<UAnimMontage> HitReactMontage;

	/// 动画播放速率（1.0 = 原速）
	UPROPERTY(EditDefaultsOnly, Category = "HitReact", meta = (ClampMin = "0.01"))
	float MontagePlayRate = 1.0f;

	/// 当前播放任务（用于结束回调里解绑）
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	/// Montage 播完 / 被打断 → 恢复移动并结束能力
	UFUNCTION()
	void OnHitReactFinished();
};