#include "Combat/GA_Attack.h"
#include "Player/PlayerMaster.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Combat/AlphaGameplayTags.h"

UGA_Attack::UGA_Attack()
{
    // 连招全程存活，状态挂在实例成员上
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    // 连招是玩家主动输入 → 本地预测执行：客户端立即响应，服务器复核。两端各跑一份 ActivateAbility，各自管理自己的连招状态机
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    // 能力标签
    FGameplayTagContainer AbilityAssetTags;
    AbilityAssetTags.AddTag(AlphaGameplayTags::Ability_Combo);
    SetAssetTags(AbilityAssetTags);

    // 激活期间挂载/阻塞的标签（连招期间不许重复激活）
    ActivationOwnedTags.AddTag(AlphaGameplayTags::State_Combo);
    ActivationBlockedTags.AddTag(AlphaGameplayTags::State_Combo);

    // 左右键通过 GameplayEvent 自动激活本能力
    FAbilityTriggerData LeftTrigger;
    LeftTrigger.TriggerTag = AlphaGameplayTags::Input_Attack_Left;
    LeftTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(LeftTrigger);

    FAbilityTriggerData RightTrigger;
    RightTrigger.TriggerTag = AlphaGameplayTags::Input_Attack_Right;
    RightTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(RightTrigger);

}


void UGA_Attack::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 判断起手是左键还是右键
    EAttackHand StartHand = EAttackHand::Left;
    if (TriggerEventData && TriggerEventData->EventTag.MatchesTagExact(AlphaGameplayTags::Input_Attack_Right))
    {
        StartHand = EAttackHand::Right;
    }

    // 重置连招状态
    ComboStep = 0;
    bInDerivedBranch = false;
    bDerivedWindowOpen = false;
    bHasBufferedInput = false;
    bInComboWindow = false;
    bComboWindowPassed = false;

    // 不要立即创建 WaitGameplayEvent，延迟到下一帧，避免捕获到激活事件
    InputListenerTimerHandle = GetWorld()->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(this, &UGA_Attack::SetupInputListeners));

    // 播放第一段
    PlaySection(StartHand);
}

// 播放当前段
void UGA_Attack::PlaySection(EAttackHand Hand)
{

    
    if (APlayerMaster* Player = GetPlayerMaster())
    {
        // 每段开始时清除「可中断」标记，防止上一段后摇通知残留到本段攻击段
        Player->ClearComboInterruptFlag();  

        // 每段连击开始时，按输入方向平滑转向攻击朝向
        Player->FaceAttackDirection();     
    }
        
    UAnimMontage* Montage = GetMontage(Hand);
    if (!Montage)
    {
        EndCombo();
        return;
    }

    WindowEventTasks.Reset();   // 每段开始清空窗口任务列表

    bInComboWindow = false;        
    bComboWindowPassed = false;
    bDerivedWindowOpen = false;      // 每段开始时重置，防止第2段衍生窗口状态泄漏到后续段
    bBufferWindowOpen = false;   // 每段开始重置缓冲窗口

    CurrentMontageTask = PlayMontageTask(Montage, 1.0f);
    if (CurrentMontageTask)
    {
        CurrentMontageTask->OnCompleted.AddDynamic(this, &UGA_Attack::OnSectionCompleted);
        CurrentMontageTask->OnCancelled.AddDynamic(this, &UGA_Attack::OnSectionCompleted);
        CurrentMontageTask->OnInterrupted.AddDynamic(this, &UGA_Attack::OnSectionCompleted);
        CurrentMontageTask->ReadyForActivation();
    }

    // 每段都监听「连招接续窗口」的打开与关闭
    UAbilityTask_WaitGameplayEvent* WaitWindow =
        UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AlphaGameplayTags::Combo_Window, nullptr, false, true);
    WaitWindow->EventReceived.AddDynamic(this, &UGA_Attack::OnComboWindowBegin);
    WaitWindow->ReadyForActivation();
    WindowEventTasks.Add(WaitWindow);

    UAbilityTask_WaitGameplayEvent* WaitWindowEnd =
        UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AlphaGameplayTags::Combo_WindowEnd, nullptr, false, true);
    WaitWindowEnd->EventReceived.AddDynamic(this, &UGA_Attack::OnComboWindowEnd);
    WaitWindowEnd->ReadyForActivation();
    WindowEventTasks.Add(WaitWindowEnd);

    // 监听「缓冲窗口打开」：只有缓冲窗口打开后，攻击段内的按键才会缓冲（收窄提前量）
    UAbilityTask_WaitGameplayEvent* WaitBufferWindow =
        UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AlphaGameplayTags::Combo_BufferWindowOpen, nullptr, true, true);
    WaitBufferWindow->EventReceived.AddDynamic(this, &UGA_Attack::OnBufferWindowOpen);
    WaitBufferWindow->ReadyForActivation();
    WindowEventTasks.Add(WaitBufferWindow);

    // 每段都监听「中断请求」（移动/跳跃等触发），由本 GA 判断缓冲优先级后决定是否中断
    UAbilityTask_WaitGameplayEvent* WaitInterrupt =
        UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AlphaGameplayTags::Combo_InterruptRequest, nullptr, false, true);
    WaitInterrupt->EventReceived.AddDynamic(this, &UGA_Attack::OnInterruptRequest);
    WaitInterrupt->ReadyForActivation();
    WindowEventTasks.Add(WaitInterrupt);

    // 仅基础第2段监听「衍生窗口」的打开与关闭
    if (!bInDerivedBranch && ComboStep == 1)
    {
        UAbilityTask_WaitGameplayEvent* WaitDerived =
            UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AlphaGameplayTags::Combo_DerivedWindow, nullptr, true, true);
        WaitDerived->EventReceived.AddDynamic(this, &UGA_Attack::OnDerivedWindowOpened);
        WaitDerived->ReadyForActivation();
        WindowEventTasks.Add(WaitDerived);

        UAbilityTask_WaitGameplayEvent* WaitDerivedEnd =
            UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AlphaGameplayTags::Combo_DerivedWindowEnd, nullptr, true, true);
        WaitDerivedEnd->EventReceived.AddDynamic(this, &UGA_Attack::OnDerivedWindowClosed);
        WaitDerivedEnd->ReadyForActivation();
        WindowEventTasks.Add(WaitDerivedEnd);
    }
}

// 按「当前段号 + 是否衍生」取对应 Montage
UAnimMontage* UGA_Attack::GetMontage(EAttackHand Hand) const
{
    const TArray<TObjectPtr<UAnimMontage>>& Montages =
        (Hand == EAttackHand::Left) ? LeftComboMontages : RightComboMontages;

    // 基础段索引 0~3，衍生段索引 4~5
    const int32 Index = bInDerivedBranch ? (4 + ComboStep) : ComboStep;
    return Montages.IsValidIndex(Index) ? Montages[Index] : nullptr;
}

// 一段播完
void UGA_Attack::OnSectionCompleted()
{

    if (bHasBufferedInput && !bComboWindowPassed)
    {
        // 兜底：缓冲输入因故（如 AnimNotify 漏放）未在可打断点消费，这里补接
        bHasBufferedInput = false;
        AdvanceCombo(BufferedHand, false);
    }
    else
    {
        // 收招完整播完且无缓冲输入 → 重置连招
        EndCombo();
    }
}

// 收到攻击输入（整个连招期间持续监听）
void UGA_Attack::OnAttackInput(FGameplayEventData Payload)
{

    const bool bLeft = Payload.EventTag.MatchesTagExact(AlphaGameplayTags::Input_Attack_Left);
    const EAttackHand Hand = bLeft ? EAttackHand::Left : EAttackHand::Right;

    if (bInComboWindow)
    {
        // 接续窗口内 → 立即接下一段
        AdvanceCombo(Hand, false);
    }
    else if (!bComboWindowPassed && bBufferWindowOpen)   // 攻击段内、且缓冲窗口已打开 → 才缓冲
    {
        bHasBufferedInput = true;
        BufferedHand = Hand;
    }
    else
    {
        // 接续窗口已过（收招后段）→ 忽略，除非衍生窗口打开
        if (bDerivedWindowOpen)
        {
            AdvanceCombo(Hand, true);
        }
    }
}

// 推进到下一段（核心分支决策）
void UGA_Attack::AdvanceCombo(EAttackHand Hand, bool bPressedInDerivedWindow)
{

    // 边界保护：已在最后一段（基础第4段 ComboStep==3 / 衍生第2段 ComboStep==1），
    // 不允许再推进，直接结束连招。否则 ComboStep 会溢出到 4，取到衍生段的 Montage。
    if ((!bInDerivedBranch && ComboStep >= 3) || (bInDerivedBranch && ComboStep >= 1))
    {
        EndCombo();
        return;
    }

    // 此时 ComboStep 仍是「刚播完的段号」，据此决定下一段
    if (!bInDerivedBranch)
    {
        if (ComboStep == 1)
        {
            // 刚播完基础第2段 → 分支决策
            if (bPressedInDerivedWindow)
            {
                // 停顿后按 → 进入衍生分支
                bInDerivedBranch = true;
                ComboStep = 0;             // 衍生第1段
            }
            else
            {
                // 快速连按 → 基础第3段
                ComboStep = 2;
            }
        }
        else
        {
            ComboStep++;                    // 第1→2，或第3→4
        }
    }
    else
    {
        ComboStep++;                        // 衍生1→2
    }

    // 先结束本段所有窗口事件监听，防止旧段 NotifyEnd 事件（在 PlaySection 播放新 Montage 时同步触发）泄漏到新段
    for (TObjectPtr<UAbilityTask_WaitGameplayEvent>& Task : WindowEventTasks)
    {
        if (Task)
        {
            Task->EndTask();
        }
    }
    WindowEventTasks.Reset();

    // 停止当前 Montage（先解绑回调，避免打断时误触发 OnSectionCompleted）
    if (CurrentMontageTask)
    {
        CurrentMontageTask->OnCompleted.RemoveDynamic(this, &UGA_Attack::OnSectionCompleted);
        CurrentMontageTask->OnCancelled.RemoveDynamic(this, &UGA_Attack::OnSectionCompleted);
        CurrentMontageTask->OnInterrupted.RemoveDynamic(this, &UGA_Attack::OnSectionCompleted);
        CurrentMontageTask->EndTask();
        CurrentMontageTask = nullptr;
    }

    PlaySection(Hand);
}

// 连招接续窗口打开
void UGA_Attack::OnComboWindowBegin(FGameplayEventData Payload)
{
    if (bInComboWindow) return;   // 防重复 Begin
    bInComboWindow = true;

    // 攻击段内提前按了键（缓冲），此刻立即接下一段
    if (bHasBufferedInput)
    {
        bHasBufferedInput = false;
        AdvanceCombo(BufferedHand, false);
    }
}

// 连招接续窗口关闭：过了这个状态不能再接下一段
void UGA_Attack::OnComboWindowEnd(FGameplayEventData Payload)
{
    // 守卫：上一段打断时泄漏过来的 WindowEnd，到达时本段接续窗口还没 Begin（bInComboWindow 仍为 false），
    // 直接丢弃，避免把新段窗口提前关闭。
    if (!bInComboWindow) return;

    bInComboWindow = false;
    bComboWindowPassed = true;
}

// 衍生窗口打开（第2段收招 AnimNotify 触发）
void UGA_Attack::OnDerivedWindowOpened(FGameplayEventData Payload)
{
    bDerivedWindowOpen = true;

    bInComboWindow = false;
    bComboWindowPassed = true;
}

// 衍生窗口关闭
void UGA_Attack::OnDerivedWindowClosed(FGameplayEventData Payload)
{
    bDerivedWindowOpen = false;
}

// 缓冲窗口打开：此后到接续窗口打开之前，攻击输入才被缓冲
void UGA_Attack::OnBufferWindowOpen(FGameplayEventData Payload)
{
    bBufferWindowOpen = true;
}

// 收到中断请求（移动/跳跃触发）：缓冲攻击优先于中断
void UGA_Attack::OnInterruptRequest(FGameplayEventData Payload)
{
    // 有尚未消费的缓冲攻击（且接续窗口未过）→ 忽略中断，
    // 让缓冲在接续窗口打开时正常接下一段（缓冲优先）
    if (bHasBufferedInput && !bComboWindowPassed)
    {
        return;
    }

    // 无有效缓冲 → 真正中断（EndCombo 内部会停 Montage + EndAbility）
    EndCombo();
}

// 中断重置
void UGA_Attack::EndCombo()
{

    // 复位中断标记
    if (APlayerMaster* Player = GetPlayerMaster())
        Player->ClearComboInterruptFlag();

    // 先解绑并结束当前 Montage 任务，避免 EndAbility 停 Montage 时
    // 触发 OnCancelled/OnInterrupted 再次回调 OnSectionCompleted（二次 EndCombo）
    if (CurrentMontageTask)
    {
        CurrentMontageTask->OnCompleted.RemoveDynamic(this, &UGA_Attack::OnSectionCompleted);
        CurrentMontageTask->OnCancelled.RemoveDynamic(this, &UGA_Attack::OnSectionCompleted);
        CurrentMontageTask->OnInterrupted.RemoveDynamic(this, &UGA_Attack::OnSectionCompleted);
        CurrentMontageTask = nullptr;
    }

    // 重置状态变量
    ComboStep = 0;
    bInDerivedBranch = false;
    bDerivedWindowOpen = false;
    bHasBufferedInput = false;
    bInComboWindow = false;
    bComboWindowPassed = false;
    bBufferWindowOpen = false;

    // 清除可能残留的「延迟注册监听器」定时器，
    // 防止能力已结束（如 Montage 数组配空）时，下一帧仍创建输入监听任务
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(InputListenerTimerHandle);
    }

    // 结束能力（触发 GAS 清理）
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

//设置输入监听器
void UGA_Attack::SetupInputListeners()
{
    //能力已结束就不再创建监听任务
    if (!IsActive()) return;

    // 整个连招期间持续监听左右键（不自动停止）
    UAbilityTask_WaitGameplayEvent* WaitLeft =
        UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AlphaGameplayTags::Input_Attack_Left, nullptr, false, true);
    WaitLeft->EventReceived.AddDynamic(this, &UGA_Attack::OnAttackInput);
    WaitLeft->ReadyForActivation();

    UAbilityTask_WaitGameplayEvent* WaitRight =
        UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AlphaGameplayTags::Input_Attack_Right, nullptr, false, true);
    WaitRight->EventReceived.AddDynamic(this, &UGA_Attack::OnAttackInput);
    WaitRight->ReadyForActivation();
}

