#include "Combat/GA_EnemyAttack.h"
#include "Enemy/EnemyCharacter.h"
#include "Weapon/WeaponComponent.h"
#include "Combat/AlphaGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_EnemyAttack::UGA_EnemyAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 敌人 AI 只存在于服务器（AIController 不复制）→ 能力只在服务器跑。
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// 能力标签（便于按标签取消/查询）
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(AlphaGameplayTags::Ability_EnemyAttack);
	SetAssetTags(AbilityAssetTags);

	// 攻击期间挂「攻击中」状态，同时阻塞重复激活
	ActivationOwnedTags.AddTag(AlphaGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(AlphaGameplayTags::State_Attacking);
}


void UGA_EnemyAttack::ActivateAbility(
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

	// 攻击前把武器从背部切到手上（巡逻时武器背在背上）
	if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UWeaponComponent* Weapon = Enemy->GetWeaponComponent())
		{
			if (!Weapon->IsWeaponDrawn())
			{
				const FWeaponConfig* Config = Weapon->GetCurrentWeaponConfig();
				UAnimMontage* DrawMontage = Config ? Config->DrawWeaponMontage : nullptr;

				if (DrawMontage)
				{
					DrawTask = PlayMontageTask(DrawMontage, 1.f);
					if (DrawTask)
					{
						DrawTask->OnCompleted.AddDynamic(this, &UGA_EnemyAttack::OnDrawFinished);
						DrawTask->OnInterrupted.AddDynamic(this, &UGA_EnemyAttack::OnDrawFinished);
						DrawTask->OnCancelled.AddDynamic(this, &UGA_EnemyAttack::OnDrawFinished);
						DrawTask->ReadyForActivation();
						bDrawPending = true;
						return;   // 等拔刀播完，由 OnDrawFinished 接着攻击
					}
				}

				// 兜底：没配拔刀 Montage → 瞬时切到手上
				Weapon->AttachWeaponToHand();
			}
		}
	}

	StartAttack();
}

// 拔刀结束 → 开始攻击
void UGA_EnemyAttack::OnDrawFinished()
{
	if (!bDrawPending) return;   // 能力已结束，不再继续攻击
	bDrawPending = false;

	if (DrawTask)
	{
		DrawTask->OnCompleted.RemoveDynamic(this, &UGA_EnemyAttack::OnDrawFinished);
		DrawTask->OnInterrupted.RemoveDynamic(this, &UGA_EnemyAttack::OnDrawFinished);
		DrawTask->OnCancelled.RemoveDynamic(this, &UGA_EnemyAttack::OnDrawFinished);
		DrawTask = nullptr;
	}

	StartAttack();
}


// 播放攻击 Montage
void UGA_EnemyAttack::StartAttack()
{
	if (!AttackMontage)
	{
		EndAttack();
		return;
	}

	MontageTask = PlayMontageTask(AttackMontage, MontagePlayRate);
	if (!MontageTask)
	{
		EndAttack();
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UGA_EnemyAttack::OnAttackFinished);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_EnemyAttack::OnAttackFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_EnemyAttack::OnAttackFinished);
	MontageTask->ReadyForActivation();
}


void UGA_EnemyAttack::OnAttackFinished()
{
	EndAttack();
}


void UGA_EnemyAttack::EndAttack()
{
	// 关掉武器命中盒（防止打断时残留开启）
	if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UWeaponComponent* Weapon = Enemy->GetWeaponComponent())
		{
			Weapon->DisableWeaponHitbox();
		}
	}

	// 清理拔刀任务（若还在播）
	if (DrawTask)
	{
		DrawTask->OnCompleted.RemoveDynamic(this, &UGA_EnemyAttack::OnDrawFinished);
		DrawTask->OnInterrupted.RemoveDynamic(this, &UGA_EnemyAttack::OnDrawFinished);
		DrawTask->OnCancelled.RemoveDynamic(this, &UGA_EnemyAttack::OnDrawFinished);
		DrawTask = nullptr;
	}
	bDrawPending = false;

	// 只解绑回调 + 清引用；停 Montage 交给 EndAbility 触发（项目既有约定，见 montage-stop-convention）
	if (MontageTask)
	{
		MontageTask->OnCompleted.RemoveDynamic(this, &UGA_EnemyAttack::OnAttackFinished);
		MontageTask->OnCancelled.RemoveDynamic(this, &UGA_EnemyAttack::OnAttackFinished);
		MontageTask->OnInterrupted.RemoveDynamic(this, &UGA_EnemyAttack::OnAttackFinished);
		MontageTask = nullptr;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}