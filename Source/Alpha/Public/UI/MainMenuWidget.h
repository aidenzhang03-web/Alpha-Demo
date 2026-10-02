#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class AMenuPlayerController;
class UButton;
class UComboBoxString;
class USlider;
class UWidgetSwitcher;

/**
 * 主菜单界面基类。
 * 只做界面逻辑（切页、按钮转发），视觉布局在 WBP_MainMenu 里完成。
 */
UCLASS()
class ALPHA_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 三页容器，子项顺序必须与下方 EMenuPage 一致 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> PageSwitcher;

	/** 主菜单页：开始游戏 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> StartButton;

	/** 主菜单页：继续游戏（存档系统落地前置灰） */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> ContinueButton;

	/** 主菜单页：设置 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> SettingsButton;

	/** 主菜单页：键位说明 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> KeyBindingsButton;

	/** 主菜单页：退出游戏 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

	/** 设置页返回按钮，WBP 中可不放（不放则需在该页另设返回方式） */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackFromSettingsButton;

	/** 键位说明页返回按钮，同上 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackFromKeyButton;

	/** 设置页：分辨率下拉，条目形如 "1920 x 1080" */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ResolutionCombo;

	/** 设置页：窗口模式下拉（全屏 / 窗口化全屏 / 窗口化） */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> WindowModeCombo;

	/** 设置页：主音量滑条（0 ~ 1） */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<USlider> VolumeSlider;

	/** 设置页：应用按钮，点击后写入并保存 GameUserSettings */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> ApplySettingsButton;

private:
	/** 页索引约定，必须与 WBP_MainMenu 中 PageSwitcher 的子项顺序一致 */
	enum EMenuPage : int32
	{
		Page_MainMenu = 0,     // 主菜单
		Page_Settings = 1,     // 设置
		Page_KeyBindings = 2   // 键位说明
	};

	/**
	 * 取本界面所属的菜单控制器。
	 * @return 菜单控制器指针；不在菜单关卡（或已脱离控制器）时返回 nullptr
	 */
	AMenuPlayerController* GetMenuController() const;

	/** 开始游戏 */
	UFUNCTION()
	void HandleStartClicked();

	/** 继续游戏（当前为空实现） */
	UFUNCTION()
	void HandleContinueClicked();

	/** 切到设置页 */
	UFUNCTION()
	void HandleSettingsClicked();

	/** 切到键位说明页 */
	UFUNCTION()
	void HandleKeyBindingsClicked();

	/** 退出游戏 */
	UFUNCTION()
	void HandleQuitClicked();

	/** 设置页 / 键位页返回主菜单 */
	UFUNCTION()
	void HandleBackClicked();

	/** 把引擎当前设置同步到设置页控件（界面构造时调一次，保证显示的是生效值） */
	void RefreshSettingsPanel();

	/** 把设置页控件上的选择写入引擎 GameUserSettings 并保存到 ini */
	void ApplySettingsFromPanel();

	/** 音量滑条变化：即时生效，听感需要立刻反馈 */
	UFUNCTION()
	void HandleVolumeChanged(float Value);

	/** 应用按钮：保存并应用分辨率 / 窗口模式 */
	UFUNCTION()
	void HandleApplyClicked();
};
