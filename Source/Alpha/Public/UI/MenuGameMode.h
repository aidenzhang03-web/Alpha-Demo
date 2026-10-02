#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MenuGameMode.generated.h"

/**
 * 菜单关卡专用 GameMode。
 * 只让本地玩家拿到负责创建主菜单界面的 PlayerController；
 * 不生成 Pawn —— 关卡是空场景，需要相机时直接在关卡里摆 Camera Actor。
 */
UCLASS()
class ALPHA_API AMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMenuGameMode();

protected:
	/**
	 * 菜单关不生成 Pawn，因此直接禁止玩家重开。
	 * @param Player 请求重开的控制器（此实现不使用）
	 * @return 恒为 false，表示不需要走 RestartPlayer 流程
	 */
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;
};
