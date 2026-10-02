#include "UI/MainMenuWidget.h"

#include "AudioDevice.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/MenuPlayerController.h"

namespace
{
	/** 分辨率条目文本："1920 x 1080"，解析时按同一格式反解 */
	FString MakeResolutionLabel(const FIntPoint& Resolution)
	{
		return FString::Printf(TEXT("%d x %d"), Resolution.X, Resolution.Y);
	}

	/** @return 选项为空或格式不符时返回 false，OutResolution 保持原值 */
	bool ParseResolutionLabel(const FString& Label, FIntPoint& OutResolution)
	{
		FString WidthText, HeightText;
		if (!Label.Split(TEXT(" x "), &WidthText, &HeightText))
		{
			return false;
		}

		const int32 Width = FCString::Atoi(*WidthText);
		const int32 Height = FCString::Atoi(*HeightText);
		if (Width <= 0 || Height <= 0)
		{
			return false;
		}

		OutResolution = FIntPoint(Width, Height);
		return true;
	}

	// 窗口模式下拉的固定条目。用文本而非索引匹配，避免 WBP 里调换条目顺序后设置错位
	const TCHAR* const WindowModeFullscreen = TEXT("全屏");
	const TCHAR* const WindowModeWindowedFullscreen = TEXT("窗口化全屏");
	const TCHAR* const WindowModeWindowed = TEXT("窗口化");

	EWindowMode::Type ParseWindowModeLabel(const FString& Label)
	{
		if (Label == WindowModeWindowed) return EWindowMode::Windowed;
		if (Label == WindowModeWindowedFullscreen) return EWindowMode::WindowedFullscreen;
		return EWindowMode::Fullscreen;
	}

	FString MakeWindowModeLabel(EWindowMode::Type Mode)
	{
		switch (Mode)
		{
		case EWindowMode::Windowed:           return WindowModeWindowed;
		case EWindowMode::WindowedFullscreen: return WindowModeWindowedFullscreen;
		default:                              return WindowModeFullscreen;
		}
	}
}

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 动态多播必须用 AddDynamic 绑定 UFUNCTION 回调；控件缺失时跳过，
	// 避免因为 WBP 少放一个按钮就整个界面崩掉
	if (StartButton)            StartButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleStartClicked);
	if (ContinueButton)         ContinueButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleContinueClicked);
	if (SettingsButton)         SettingsButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleSettingsClicked);
	if (KeyBindingsButton)      KeyBindingsButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleKeyBindingsClicked);
	if (QuitButton)             QuitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleQuitClicked);
	if (BackFromSettingsButton) BackFromSettingsButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleBackClicked);
	if (BackFromKeyButton)      BackFromKeyButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleBackClicked);
	if (ApplySettingsButton)    ApplySettingsButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleApplyClicked);
	if (VolumeSlider)           VolumeSlider->OnValueChanged.AddDynamic(this, &UMainMenuWidget::HandleVolumeChanged);

	// 存档系统尚未实现，CanContinue 恒为 false，先把按钮置灰占位；
	// 等存档落地后自行亮起，不需要再改 WBP
	const AMenuPlayerController* MenuPC = GetMenuController();
	if (ContinueButton)
	{
		ContinueButton->SetIsEnabled(MenuPC && MenuPC->CanContinue());
	}

	// 同步一次设置页控件，保证切到设置页时显示的是当前生效值
	RefreshSettingsPanel();

	if (PageSwitcher)
	{
		PageSwitcher->SetActiveWidgetIndex(Page_MainMenu);
	}
}

void UMainMenuWidget::NativeDestruct()
{
	// 解绑：按钮控件可能比本界面活得久，委托里若留着悬空对象会崩溃
	if (StartButton)            StartButton->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandleStartClicked);
	if (ContinueButton)         ContinueButton->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandleContinueClicked);
	if (SettingsButton)         SettingsButton->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandleSettingsClicked);
	if (KeyBindingsButton)      KeyBindingsButton->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandleKeyBindingsClicked);
	if (QuitButton)             QuitButton->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandleQuitClicked);
	if (BackFromSettingsButton) BackFromSettingsButton->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandleBackClicked);
	if (BackFromKeyButton)      BackFromKeyButton->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandleBackClicked);
	if (ApplySettingsButton)    ApplySettingsButton->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandleApplyClicked);
	if (VolumeSlider)           VolumeSlider->OnValueChanged.RemoveDynamic(this, &UMainMenuWidget::HandleVolumeChanged);

	Super::NativeDestruct();
}

AMenuPlayerController* UMainMenuWidget::GetMenuController() const
{
	return Cast<AMenuPlayerController>(GetOwningPlayer());
}

void UMainMenuWidget::HandleStartClicked()
{
	// 转发给控制器：界面不关心切关卡用 ServerTravel 还是 OpenLevel
	if (AMenuPlayerController* MenuPC = GetMenuController())
	{
		MenuPC->StartGame();
	}
}

void UMainMenuWidget::HandleContinueClicked()
{
	if (AMenuPlayerController* MenuPC = GetMenuController())
	{
		MenuPC->ContinueGame();
	}
}

void UMainMenuWidget::HandleSettingsClicked()
{
	if (PageSwitcher)
	{
		PageSwitcher->SetActiveWidgetIndex(Page_Settings);
	}
}

void UMainMenuWidget::HandleKeyBindingsClicked()
{
	if (PageSwitcher)
	{
		PageSwitcher->SetActiveWidgetIndex(Page_KeyBindings);
	}
}

void UMainMenuWidget::HandleBackClicked()
{
	if (PageSwitcher)
	{
		PageSwitcher->SetActiveWidgetIndex(Page_MainMenu);
	}
}

void UMainMenuWidget::HandleQuitClicked()
{
	if (AMenuPlayerController* MenuPC = GetMenuController())
	{
		MenuPC->QuitGame();
	}
}

void UMainMenuWidget::RefreshSettingsPanel()
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;

	if (ResolutionCombo)
	{
		ResolutionCombo->ClearOptions();

		// 只列本机支持的全屏分辨率，避免往列表里塞无效尺寸
		TArray<FIntPoint> SupportedResolutions;
		UKismetSystemLibrary::GetSupportedFullscreenResolutions(SupportedResolutions);

		if (Settings)
		{
			// 窗口化时窗口尺寸往往不在全屏支持列表里：补进去并排序，
			// 否则下面的 SetSelectedOption 找不到匹配项，下拉会显示空白
			SupportedResolutions.AddUnique(Settings->GetScreenResolution());
			SupportedResolutions.Sort([](const FIntPoint& A, const FIntPoint& B)
				{
					return A.X != B.X ? A.X < B.X : A.Y < B.Y;
				});
		}

		for (const FIntPoint& Resolution : SupportedResolutions)
		{
			ResolutionCombo->AddOption(MakeResolutionLabel(Resolution));
		}

		if (Settings)
		{
			ResolutionCombo->SetSelectedOption(MakeResolutionLabel(Settings->GetScreenResolution()));
		}
	}

	if (WindowModeCombo)
	{
		WindowModeCombo->ClearOptions();
		WindowModeCombo->AddOption(WindowModeFullscreen);
		WindowModeCombo->AddOption(WindowModeWindowedFullscreen);
		WindowModeCombo->AddOption(WindowModeWindowed);

		if (Settings)
		{
			WindowModeCombo->SetSelectedOption(MakeWindowModeLabel(Settings->GetFullscreenMode()));
		}
	}

	if (VolumeSlider)
	{
		// GetTransientPrimaryVolume() 带 check(IsInAudioThread())，游戏线程读会断言，
		// 所以不回读设备音量、默认给满；音量持久化等存档系统落地后统一处理
		VolumeSlider->SetValue(1.f);
	}
}

void UMainMenuWidget::ApplySettingsFromPanel()
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings) return;

	if (ResolutionCombo)
	{
		FIntPoint Resolution;
		if (ParseResolutionLabel(ResolutionCombo->GetSelectedOption(), Resolution))
		{
			Settings->SetScreenResolution(Resolution);
		}
	}

	if (WindowModeCombo)
	{
		Settings->SetFullscreenMode(ParseWindowModeLabel(WindowModeCombo->GetSelectedOption()));
	}

	// ApplySettings 内部会无条件保存（见 GameUserSettings.cpp:600）；
	// 传 true 表示允许命令行启动参数（如 -ResX=）覆盖玩家设置
	Settings->ApplySettings(true);
}

void UMainMenuWidget::HandleVolumeChanged(float Value)
{
	if (!GEngine) return;

	// SetTransientPrimaryVolume 是运行时主音量，与 GameUserSettings 无关，
	// 所以滑条拖动即时生效（不作保存）；注意其 Getter 只能在音频线程调用
	if (FAudioDevice* AudioDevice = GEngine->GetMainAudioDevice().GetAudioDevice())
	{
		AudioDevice->SetTransientPrimaryVolume(Value);
	}
}

void UMainMenuWidget::HandleApplyClicked()
{
	ApplySettingsFromPanel();
}
