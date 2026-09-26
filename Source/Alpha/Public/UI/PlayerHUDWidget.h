#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHUDWidget.generated.h"

class AAlphaGameState;
class UProgressBar;
class APlayerMaster;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;

/*
* 玩家HUD基类，监听 GAS 属性变化
* 视觉布局在 Widget Blueprint 子类（WBP_PlayerHUD）中完成
*/

UCLASS()
class ALPHA_API UPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// 绑定玩家并注册属性监听。由 PlayerMaster 在创建 HUD 后调用一次。
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void InitializeHUD(APlayerMaster* InPlayer);

    // 失败结算界面
    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void OnGameOverUI();


protected:

    // 三个进度条（Widget Blueprint 中同名控件自动绑定）
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UProgressBar> HealthBar;  // 生命值

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UProgressBar> ManaBar;  // 法力值

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UProgressBar> StaminaBar;  // 耐力值

    // 属性变化回调（事件驱动，不在 Tick 轮询）
    void OnHealthChanged(const FOnAttributeChangeData& Data);
    void OnManaChanged(const FOnAttributeChangeData& Data);
    void OnStaminaChanged(const FOnAttributeChangeData& Data);

    // 初次刷新所有条
    void RefreshAllBars();

    // GameState 失败状态变化回调。服务器置位与客户端收到复制时各触发一次。 
    UFUNCTION()
    void HandleGameOverChanged();

private:
    TWeakObjectPtr<APlayerMaster> OwningPlayer;      // 弱引用，避免悬空

    TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

    bool bInitialized = false;                       // 防重复绑定

    TWeakObjectPtr<AAlphaGameState> CachedGameState;   // 弱引用，避免悬空
};