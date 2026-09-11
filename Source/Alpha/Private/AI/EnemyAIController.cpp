#include "AI/EnemyAIController.h"
#include "Player/PlayerMaster.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "NavigationSystem.h"
#include "TimerManager.h"   



AEnemyAIController::AEnemyAIController()
{ 
	// 视觉感知：AAIController 已内置 PerceptionComponent 成员
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
	SightConfig->SetMaxAge(5.f);                       // 目标消失后仍"记得" x 秒
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	PerceptionComponent->ConfigureSense(*SightConfig);
	PerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
}



void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	// 在 BeginPlay 绑定，避免构造函数里绑到 CDO
	if (PerceptionComponent)
		PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
			this, &AEnemyAIController::OnTargetPerceptionUpdated);
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


// 更新巡逻位置
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


// 更新视觉感知目标
void AEnemyAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor) return;
	if (!Actor->IsA(APlayerMaster::StaticClass())) return;   // 只关心玩家

	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB) return;   // 行为树还没跑 / 黑板未绑定

	if (Stimulus.WasSuccessfullySensed())
	{
		BB->SetValueAsObject(TEXT("TargetActor"), Actor);
	}
	else if (BB->GetValueAsObject(TEXT("TargetActor")) == Actor)
	{
		// 仅当当前目标就是它时才清空，避免误清切换后的新目标
		BB->ClearValue(TEXT("TargetActor"));
	}
}