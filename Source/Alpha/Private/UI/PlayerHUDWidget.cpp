#include "UI/PlayerHUDWidget.h"
#include "Components/ProgressBar.h"
#include "GameFramework/PlayerController.h"
#include "Player/PlayerMaster.h"
#include "Combat/AlphaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
 

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

    // 订阅自己的死亡状态。面板显隐改由「自己是否死亡」驱动，
    // 而不是全局的 GameState::bGameOver —— 这样各自独立复活、互不影响。
    // OwningPlayer 是 HUD 持有者本人，所以只会收到自己的状态变化。
    OwningPlayer->OnDeadStateChanged.AddDynamic(this, &UPlayerHUDWidget::HandleDeadStateChanged);

    // 只在真的死了时才补一次：HUD 创建可能晚于死亡（例如中途加入的玩家），
    // 而委托只能收到「之后」的变化，收不到已经发生的那次。
    // 活着时刻意不调用 —— HandleDeadStateChanged(false) 会触发 OnReviveUI，
    // 将来那里若加了复活特效/音效，游戏开局会误触发一次。
    if(OwningPlayer->IsDead())
        HandleDeadStateChanged(true);
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

void UPlayerHUDWidget::HandleDeadStateChanged(bool bIsDead)
{
    // HUD 本就只在本地控制器的 Pawn 上创建（见 APlayerMaster::TryCreateHUD），
    // 这里再判一次属防御性写法：多播会覆盖所有端，远端 Pawn 的广播不该影响本地界面。
    if (!OwningPlayer.IsValid() || !OwningPlayer->IsLocallyControlled()) return;

    APlayerController* PC = Cast<APlayerController>(OwningPlayer->GetController());
    if (!PC) return;

    if (bIsDead)
    {
        PC->SetShowMouseCursor(true);
        PC->SetInputMode(FInputModeUIOnly());
        OnGameOverUI();      // 显示个人死亡面板
    }
    else
    {
        PC->SetShowMouseCursor(false);
        PC->SetInputMode(FInputModeGameOnly());
        OnReviveUI();        // 隐藏面板
    }
}

void UPlayerHUDWidget::RequestRevive()
{
    // 转发而非让蓝图去 Cast：WBP 里直接 Self → Request Revive 就行。
    // 按钮点击天然只发生在本地玩家，所以这里不用判 IsLocallyControlled。
    if (OwningPlayer.IsValid())
    {
        OwningPlayer->RequestRevive();
    }
}