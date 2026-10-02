#include "UI/MenuGameMode.h"
#include "UI/MenuPlayerController.h"

AMenuGameMode::AMenuGameMode()
{
	// 菜单关是空场景，没有可操控的实体；置空 DefaultPawnClass 可避免默认 Pawn
	// 生成后抢占关卡相机（否则 PlayerCameraManager 会跟着 Pawn 走，背景视角乱掉）
	DefaultPawnClass = nullptr;

	// 菜单不需要血条等战斗 HUD
	HUDClass = nullptr;

	// 菜单界面由 PlayerController 创建，所以必须换成菜单专用控制器
	PlayerControllerClass = AMenuPlayerController::StaticClass();
}

bool AMenuGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	// 菜单关没有 PlayerStart，若仍走默认流程会刷 "Failed to find player start" 警告
	return false;
}
