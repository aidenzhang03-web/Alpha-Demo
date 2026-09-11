#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyAIController.generated.h"

class UBehaviorTree;
class UAISenseConfig_Sight;

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



	// ========敌人巡逻 ========
	// 巡逻半径：在当前位置周围随机游走（单位 cm）
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (ClampMin = "0"))
	float PatrolRadius = 1000.f;

	// 每隔多久换一个巡逻目标点（秒）
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (ClampMin = "0.1"))
	float PatrolInterval = 3.f;


	// ======== 敌人视觉感知 ========
	// 视觉感知配置（构造函数里创建）
	UPROPERTY(VisibleAnywhere, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	// 视觉半径（cm）
	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception", meta = (ClampMin = "0"))
	float SightRadius = 1500.f;

	// 失去目标半径（应大于 SightRadius，避免边界抖动）
	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception", meta = (ClampMin = "0"))
	float LoseSightRadius = 2000.f;

	// 视野半角（度）：60 = 左右各 60° 的水平视锥
	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception", meta = (ClampMin = "0", ClampMax = "180"))
	float PeripheralVisionAngleDegrees = 60.f;


	// 感知回调：目标进入/离开
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	virtual void BeginPlay() override;


private:

	FTimerHandle PatrolTimerHandle;

	// 随机取一个「可到达」的点，写入黑板键 PatrolLocation
	void UpdatePatrolTarget();


};
