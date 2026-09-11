#include "AI/BTTask_EnemyAttack.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Combat/AlphaGameplayTags.h"



UBTTask_EnemyAttack::UBTTask_EnemyAttack()
{
	// BT 节点显示名
	NodeName = TEXT("敌人攻击");

	// 默认攻击能力 Tag（可在行为树节点的 Details 面板里单独覆盖）
	AttackAbilityTag = AlphaGameplayTags::Ability_EnemyAttack;
}


EBTNodeResult::Type UBTTask_EnemyAttack::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 1. 取 Pawn
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!Pawn)
		return EBTNodeResult::Failed;

	// 2. 取 ASC（AEnemyCharacter 自身实现 IAbilitySystemInterface）
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn);
	UAbilitySystemComponent* ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
		return EBTNodeResult::Failed;

	// 3. 按 Tag 激活攻击能力
	FGameplayTagContainer AttackTag(AttackAbilityTag);
	if (!ASC->TryActivateAbilitiesByTag(AttackTag))
		// 激活失败（未授予 / 被 State.Attacking 阻塞）→ 失败，让 BT 走兜底分支
		return EBTNodeResult::Failed;

	// 4. 缓存上下文并监听能力结束
	CachedASC = ASC;
	CachedBTComponent = &OwnerComp;
	ASC->OnAbilityEnded.AddUObject(this, &UBTTask_EnemyAttack::OnAbilityEnded);

	// 5. 挂起：攻击期间不让 BT 继续执行，避免与移动/巡逻冲突
	return EBTNodeResult::InProgress;
}


EBTNodeResult::Type UBTTask_EnemyAttack::AbortTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		// 先解绑：CancelAbilities 会同步触发 AbilityEnded，
		// 若此时还绑着 OnAbilityEnded 会重复 FinishLatentTask
		ASC->OnAbilityEnded.RemoveAll(this);

		FGameplayTagContainer AttackTag(AttackAbilityTag);
		ASC->CancelAbilities(&AttackTag);
	}

	CachedASC = nullptr;
	CachedBTComponent = nullptr;

	return EBTNodeResult::Aborted;

}

void UBTTask_EnemyAttack::OnAbilityEnded(const FAbilityEndedData& Data)
{
	// 过滤：只处理「攻击能力」的结束（BT 上可能同时有别的能力在跑）
	if (IsValid(Data.AbilityThatEnded))
	{
		if (!Data.AbilityThatEnded->GetAssetTags().HasTag(AttackAbilityTag))
			return;
	}

	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->OnAbilityEnded.RemoveAll(this);
	}

	// 能力结束（Montage 播完 / 被打断）→ 完成挂起任务，BT 继续往下走
	if (UBehaviorTreeComponent* BTComp = CachedBTComponent.Get())
	{
		FinishLatentTask(*BTComp, EBTNodeResult::Succeeded);
	}

	CachedASC = nullptr;
	CachedBTComponent = nullptr;
}