#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MenuPlayerController.generated.h"

class UMainMenuWidget;

/**
 * 菜单关卡的玩家控制器。
 * 负责创建主菜单界面、切换 UI 输入模式，并提供菜单各按钮的入口函数。
 * 具体使用哪个界面在蓝图子类 BP_MenuPlayerController 中指定（MainMenuWidgetClass）。
 */
UCLASS()
class ALPHA_API AMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/**
	 * 开始游戏：切换到战斗关卡。
	 * 默认走 ServerTravel 并带上 ?listen，保证联机时由服务器带客户端一起切换关卡。
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void StartGame();

	/** 退出游戏。PIE 下只结束当前 PIE 会话，打包后才真正退出进程。 */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void QuitGame();

	/**
	 * 「继续游戏」是否可用。存档系统落地前恒为 false，按钮据此置灰。
	 * @return true 表示存在可读取的存档
	 */
	UFUNCTION(BlueprintPure, Category = "Menu")
	bool CanContinue() const { return bCanContinue; }

	/** 「继续游戏」按钮入口。当前为空实现，预留存档系统的扩展位。 */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	virtual void ContinueGame();

protected:
	virtual void BeginPlay() override;

	/** 主菜单界面类。在 BP_MenuPlayerController 里指定 WBP_MainMenu，避免 C++ 中硬编码资源路径 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	TSubclassOf<UMainMenuWidget> MainMenuWidgetClass;

	/** true = ServerTravel 带 listen（联机）；false = OpenLevel（仅单机调试，联机下客户端会掉线） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	bool bUseServerTravel = true;

	/** 是否可继续游戏，等存档系统实现后改由存档状态决定 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	bool bCanContinue = false;

	/** 创建出的主菜单界面，留作后续访问（例如点击开始后主动移除） */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Menu")
	TObjectPtr<UMainMenuWidget> MenuWidget;
};
