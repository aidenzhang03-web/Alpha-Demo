#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "AlphaGameState.generated.h"

/** 失败状态变化广播。动态多播以便 HUD Widget 在蓝图里直接绑定。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameOverSignature);

/**
 * 游戏全局状态：承载「是否已团灭失败」这一需要复制给所有客户端的事实。
 *
 * 为什么不用 GameMode 存：AGameMode 只在服务器实例化，客户端拿不到；
 * 而客户端也必须知道失败，才能弹结算界面。
 */
UCLASS()
class ALPHA_API AAlphaGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	// 失败状态变化时广播。服务器置位时、客户端收到复制时各触发一次。
	UPROPERTY(BlueprintAssignable, Category = "GameState")
	FOnGameOverSignature OnGameOverChanged;

	// 是否已团灭失败。 
	UFUNCTION(BlueprintCallable, Category = "GameState")
	bool IsGameOver() const { return bGameOver; }

	// 标记失败（仅服务器生效）。由 GameMode 判定团灭后调用。 
	void SetGameOver();


	/**
	 * 判定是否所有玩家都已死亡。
	 * 采用遍历 PlayerArray 而非维护计数器：结果直接来自事实，天然幂等，
	 * 玩家掉线或中途加入时不需要额外同步计数。
	 * @return 所有已生成 Pawn 的玩家都处于死亡状态时 true；无玩家时 false（避免开局误判）。
	 */
	bool AreAllPlayersDead() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	// 失败标志位。复制源，客户端靠它驱动结算界面。 
	UPROPERTY(ReplicatedUsing = OnRep_GameOver, BlueprintReadOnly, Category = "GameState")
	bool bGameOver = false;

	// 复制回调：非权威端收到状态变化后广播事件。 
	UFUNCTION()
	void OnRep_GameOver();
};