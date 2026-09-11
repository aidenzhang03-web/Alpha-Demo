#pragma once

#include "CoreMinimal.h"
#include "Combat/AlphaGameplayAbility.h"
#include "GA_EnemyAttack.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;


/*
* 敌人攻击能力：播放攻击 Montage。
* 命中判定由 Montage 里的「攻击命中窗口」通知驱动（3a 已打通）：
*  UAlphaAnimNotifyState → AEnemyCharacter::HandleAnimStateBegin → WeaponComponent->EnableWeaponHitbox()
*/


UCLASS()
class ALPHA_API UGA_EnemyAttack : public UAlphaGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_EnemyAttack();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:

	// 拔刀是否仍在进行（EndAttack 会清掉，防止能力结束后 OnDrawFinished 继续攻击）
	bool bDrawPending = false;

	// 攻击动画 Montage（在 BP_GA_EnemyAttack 里指定）
	UPROPERTY(EditDefaultsOnly, Category = "EnemyAttack")
	TObjectPtr<UAnimMontage> AttackMontage;

	// 播放速率
	UPROPERTY(EditDefaultsOnly, Category = "EnemyAttack")
	float MontagePlayRate = 1.0f;

	// 当前播放任务
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	// 拔刀任务（先拔刀、后攻击的两段式）
	TObjectPtr<UAbilityTask_PlayMontageAndWait> DrawTask;

	// Montage 播完 / 被打断 → 结束能力
	UFUNCTION()
	void OnAttackFinished();

	// 播放攻击 Montage（无需拔刀 / 拔刀完成后调用）
	void StartAttack();

	// 结束攻击（关命中盒 + 结束能力）
	void EndAttack();

	// 拔刀 Montage 播完 → 接着攻击
	UFUNCTION()
	void OnDrawFinished();

};
