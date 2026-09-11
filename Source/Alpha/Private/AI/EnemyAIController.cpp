#include "AI/EnemyAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"




AEnemyAIController::AEnemyAIController()
{ 

}


void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 运行行为树（会自动使用 BT 资产里配置的黑板）
	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}

	// 周期性更新巡逻目标点（驱动 BT 里的 MoveTo 持续移动）
	GetWorldTimerManager().SetTimer(
		PatrolTimerHandle, this, &AEnemyAIController::UpdatePatrolTarget,
		PatrolInterval, true, 0.f);
}


void AEnemyAIController::UpdatePatrolTarget()
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn) return;

	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB) return;   // 行为树没运行 / 黑板未绑定

	// 用导航系统在当前位置周围取一个「可到达」的随机点
	FVector RandomLocation;
	if (UNavigationSystemV1::K2_GetRandomReachablePointInRadius(
		this, MyPawn->GetActorLocation(), RandomLocation, PatrolRadius))
	{
		// 键名必须与黑板里的键一致（"PatrolLocation"，类型 Vector）
		BB->SetValueAsVector(TEXT("PatrolLocation"), RandomLocation);
	}
}