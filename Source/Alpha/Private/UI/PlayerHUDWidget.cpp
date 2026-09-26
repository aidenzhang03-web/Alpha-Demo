#include "UI/PlayerHUDWidget.h"
#include "Components/ProgressBar.h"
#include "GameFramework/PlayerController.h"
#include "Player/PlayerMaster.h"
#include "Combat/AlphaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "AlphaGameState.h"
 

void UPlayerHUDWidget::InitializeHUD(APlayerMaster* InPlayer)
{
    if (!InPlayer || bInitialized) return;

    UAbilitySystemComponent* ASC = InPlayer->GetAbilitySystemComponent();
    if (!ASC) return;

    const UAlphaAttributeSet* AttrSet = ASC->GetSet<UAlphaAttributeSet>();
    if (!AttrSet) return;   // 若这里返回 null，说明 ASC 没 InitAbilityActorInfo

    OwningPlayer = InPlayer;
    CachedASC = ASC;
    bInitialized = true;

    // 事件驱动：属性一变，回调里刷新对应条（符合项目约定）
    ASC->GetGameplayAttributeValueChangeDelegate(AttrSet->GetHealthAttribute())
        .AddUObject(this, &UPlayerHUDWidget::OnHealthChanged);
    ASC->GetGameplayAttributeValueChangeDelegate(AttrSet->GetManaAttribute())
        .AddUObject(this, &UPlayerHUDWidget::OnManaChanged);
    ASC->GetGameplayAttributeValueChangeDelegate(AttrSet->GetStaminaAttribute())
        .AddUObject(this, &UPlayerHUDWidget::OnStaminaChanged);

    RefreshAllBars();



    // ===== 失败结算：订阅 GameState 的失败广播 =====
    
    // 放在末尾而非开头：上面的 early return 会提前退出，绑定必须在这之后才可靠。
    if (UWorld* World = GetWorld())
    {
        if (AAlphaGameState* GS = World->GetGameState<AAlphaGameState>())
        {
            CachedGameState = GS;
            GS->OnGameOverChanged.AddDynamic(this, &UPlayerHUDWidget::HandleGameOverChanged);

            // 绑定之前就已团灭的情况必须补一次。
            // 典型场景：中途加入的玩家、或 HUD 创建晚于失败判定。
            // 委托只能收到「之后」的变化，漏掉这句会让这些玩家永远看不到结算界面。
            if (GS->IsGameOver())
            {
                HandleGameOverChanged();
            }
        }
    }
}

// 生命值
void UPlayerHUDWidget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
    if (!HealthBar || !OwningPlayer.IsValid()) return;
    const float Max = OwningPlayer->GetMaxHealth();
    HealthBar->SetPercent(Max > 0.f ? (Data.NewValue / Max) : 0.f);
}

// 法力值
void UPlayerHUDWidget::OnManaChanged(const FOnAttributeChangeData& Data)
{
    if (!ManaBar || !OwningPlayer.IsValid()) return;
    const float Max = OwningPlayer->GetMaxMana();
    ManaBar->SetPercent(Max > 0.f ? (Data.NewValue / Max) : 0.f);
}

// 耐力值
void UPlayerHUDWidget::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
    if (!StaminaBar || !OwningPlayer.IsValid()) return;
    const float Max = OwningPlayer->GetMaxStamina();
    StaminaBar->SetPercent(Max > 0.f ? (Data.NewValue / Max) : 0.f);
}

// 进度条刷新
void UPlayerHUDWidget::RefreshAllBars()
{
    if (!OwningPlayer.IsValid()) return;

    if (HealthBar)
    {
        const float Max = OwningPlayer->GetMaxHealth();
        HealthBar->SetPercent(Max > 0.f ? (OwningPlayer->GetHealth() / Max) : 0.f);
    }
    if (ManaBar)
    {
        const float Max = OwningPlayer->GetMaxMana();
        ManaBar->SetPercent(Max > 0.f ? (OwningPlayer->GetMana() / Max) : 0.f);
    }
    if (StaminaBar)
    {
        const float Max = OwningPlayer->GetMaxStamina();
        StaminaBar->SetPercent(Max > 0.f ? (OwningPlayer->GetStamina() / Max) : 0.f);
    }
}

// GameState 失败状态变化回调
void UPlayerHUDWidget::HandleGameOverChanged()
{
    // 只弹本地玩家的界面。HUD 本来只在本地控制器上创建
    // （见 APlayerMaster::PossessedBy 里的 IsLocalController 判断），这里再判一次属防御性写法。
    if (!OwningPlayer.IsValid() || !OwningPlayer->IsLocallyControlled()) return;

    // 结算界面要能被鼠标操作：切到 UI 输入模式并显示鼠标。
    // 移动 / 视角输入已在 APlayerMaster::Multi_Die 里关掉了，这里只管鼠标可见性。
    // 若结算面板不需要任何交互（纯提示文字），这一段可以删掉。
    if (APlayerController* PC = Cast<APlayerController>(OwningPlayer->GetController()))
    {
        PC->SetShowMouseCursor(true);
        PC->SetInputMode(FInputModeUIOnly());
    }

    OnGameOverUI();   // 转发给蓝图显示结算面板
}