#include "UI/MenuPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "UI/MainMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void AMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 菜单是纯 UI 交互：必须显示鼠标并把输入全交给 UMG，
	// 否则鼠标被锁进视口、按钮点不到
	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());

	if (!MainMenuWidgetClass)
	{
		// 没有配置时显式警告
		UE_LOG(LogTemp, Warning,
			TEXT("AMenuPlayerController: MainMenuWidgetClass 未配置，菜单界面不会创建。请在 BP_MenuPlayerController 的 Class Defaults 里指定 WBP_MainMenu。"));

		return;
	}

	MenuWidget = CreateWidget<UMainMenuWidget>(this, MainMenuWidgetClass);
	if (MenuWidget)
	{
		// ZOrder 给高一点，避免之后加入的 HUD 盖住菜单
		MenuWidget->AddToViewport(10);
	}
}

void AMenuPlayerController::StartGame()
{
	// ServerTravel / OpenLevel 都不是立刻生效，菜单会在切图瞬间残留在屏幕上；
	// 本 PC 随后随旧世界一起销毁，界面已无意义，先主动收起
	if (MenuWidget)
	{
		MenuWidget->RemoveFromParent();
		MenuWidget = nullptr;
	}

	// 联机必须由服务器发起切换关卡（ServerTravel），客户端会自动跟随；
	// 若改用 OpenLevel，非主机端会掉线且 GAS 属性状态全部丢失
	if (bUseServerTravel)
	{
		if (UWorld* World = GetWorld())
		{
			World->ServerTravel(TEXT("/Game/Alpha/Maps/AlphaMap?listen"));
		}
		return;
	}

	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Alpha/Maps/AlphaMap")));
}

void AMenuPlayerController::QuitGame()
{
	// PIE 下会直接结束当前 PIE 会话；打包后才是真正退出进程
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void AMenuPlayerController::ContinueGame()
{
	// 预留：存档系统尚未实现，bCanContinue 恒为 false，此处暂不做事
}
