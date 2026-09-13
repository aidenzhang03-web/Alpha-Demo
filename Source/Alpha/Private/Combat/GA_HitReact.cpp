#include "Combat/GA_HitReact.h"
#include "Enemy/EnemyCharacter.h"
#include "Combat/AlphaGameplayTags.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"


UGA_HitReact::UGA_HitReact()
{
	// 每 Actor 一个实例，便于持有 MontageTask 等成员状态
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 能力标签：供别处按 AssetTags 匹配/取消（注意 CancelAbilitiesWithTag 匹配的是 AssetTags）
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(AlphaGameplayTags::Ability_HitReact);
	SetAssetTags(AbilityAssetTags);

	// 受击打断正在进行的敌人攻击
	CancelAbilitiesWithTag.AddTag(AlphaGameplayTags::Ability_EnemyAttack);

	// 受击期间挂状态标记；同时用同一标签阻塞重复激活（防抖：受击期间不再触发受击）
	ActivationOwnedTags.AddTag(AlphaGameplayTags::State_HitReact);
	ActivationBlockedTags.AddTag(AlphaGameplayTags::State_HitReact);

	// 触发方式：监听 GameplayEvent（AEnemyCharacter::OnHealthChanged 发 Event.HitReact）
	// TriggerSource = GameplayEvent 表示「收到该事件时自动激活本能力」
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = AlphaGameplayTags::Event_HitReact;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}


void UGA_HitReact::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 受击期间锁定移动：先停 AI 寻路移动，再禁用移动组件（避免边受击边滑步)
	if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (AAIController* AICon = Cast<AAIController>(Enemy->GetController()))
		{
			AICon->StopMovement();          // 中止行为树 MoveTo 的位移
		}

		if (UCharacterMovementComponent* Move = Enemy->GetCharacterMovement())
		{
			Move->StopMovementImmediately(); // 清掉残余速度
			Move->DisableMovement();         // 受击期间不接受移动输入
		}
	}

	// 播放受击动画（复用基类封装）
	MontageTask = PlayMontageTask(HitReactMontage, MontagePlayRate);
	if (!MontageTask)
	{
		// 兜底：没配受击 Montage 时也要正确收尾（恢复移动 + 结束能力），否则敌人会卡在原地
		OnHitReactFinished();
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UGA_HitReact::OnHitReactFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_HitReact::OnHitReactFinished);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_HitReact::OnHitReactFinished);
	MontageTask->ReadyForActivation();
}


void UGA_HitReact::OnHitReactFinished()
{
	// 恢复移动（受击结束，交还给行为树/AI 控制）
	if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (!Enemy->IsDead())   // 死亡时交给布娃娃，不恢复行走
		{
			if (UCharacterMovementComponent* Move = Enemy->GetCharacterMovement())
			{
				Move->SetMovementMode(MOVE_Walking);
			}
		}
		
	}

	// 只解绑回调 + 清引用；停止 Montage 交给 EndAbility 触发的 OnDestroy(true)
	// （项目约定：不要用 EndTask 停 Montage，见 montage-stop-convention）
	if (MontageTask)
	{
		MontageTask->OnCompleted.RemoveDynamic(this, &UGA_HitReact::OnHitReactFinished);
		MontageTask->OnInterrupted.RemoveDynamic(this, &UGA_HitReact::OnHitReactFinished);
		MontageTask->OnCancelled.RemoveDynamic(this, &UGA_HitReact::OnHitReactFinished);
		MontageTask = nullptr;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}