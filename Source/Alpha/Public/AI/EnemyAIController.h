#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class UBehaviorTree;

/*
* 敌人 AI 控制器：运行行为树，并周期性更新巡逻目标点（写入黑板）。
*/

UCLASS()
class ALPHA_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:

	AEnemyAIController();

protected:

	virtual void OnPossess(APawn* InPawn) override;

	// 行为树资产（在 BP_EnemyAIController 蓝图里指定）
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	// 巡逻半径：在当前位置周围随机游走（单位 cm）
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (ClampMin = "0"))
	float PatrolRadius = 1000.f;

	// 每隔多久换一个巡逻目标点（秒）
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (ClampMin = "0.1"))
	float PatrolInterval = 3.f;


private:

	FTimerHandle PatrolTimerHandle;

	// 随机取一个「可到达」的点，写入黑板键 PatrolLocation
	void UpdatePatrolTarget();
};
