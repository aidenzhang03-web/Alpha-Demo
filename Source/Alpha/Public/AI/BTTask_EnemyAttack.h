#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "BTTask_EnemyAttack.generated.h"

class UAbilitySystemComponent;
class UBehaviorTreeComponent;
struct FAbilityEndedData;

/**
 * 行为树任务：让敌人激活攻击能力（默认 Ability.EnemyAttack）。
 *
 * 职责边界：
 *  - 只负责「发起攻击」：从 ASC 按 Tag 激活能力；
 *    命中判定由攻击 Montage 里的「攻击命中窗口」NotifyState 驱动，本任务不参与。
 *  - 返回 InProgress，挂起等待能力结束，保证攻击期间 BT 不会继续往下执行
 *    （避免攻击与移动/巡逻冲突）。能力结束（Montage 播完 / 被打断）后完成任务。
 */

UCLASS()
class ALPHA_API UBTTask_EnemyAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_EnemyAttack();

	// 执行任务：按 Tag 激活攻击能力，并挂起等待能力结束
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// 任务被中断（如移动打断）：取消正在播放的攻击能力
	virtual EBTNodeResult::Type AbortTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:

	// 攻击能力 Tag（默认 AlphaGameplayTags::Ability_EnemyAttack，可在 BT 节点里改）
	UPROPERTY(EditAnywhere, Category = "EnemyAttack")
	FGameplayTag AttackAbilityTag;

	// 能力结束回调（过滤掉非攻击能力，攻击结束后完成 latent 任务）
	void OnAbilityEnded(const FAbilityEndedData& Data);

	
private:

	// 本次任务激活所在的 ASC（弱引用，避免悬空）
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	// 本次任务所属的行为树组件（能力结束时需要用它 FinishLatentTask）
	TWeakObjectPtr<UBehaviorTreeComponent> CachedBTComponent;

};