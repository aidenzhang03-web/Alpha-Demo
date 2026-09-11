#pragma once

#include "CoreMinimal.h"
#include "Combat/AlphaGameplayAbility.h"
#include "TimerManager.h"
#include "GA_Attack.generated.h"


class APlayerMaster;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;


// 攻击手（左右键）
UENUM(BlueprintType)
enum class EAttackHand : uint8
{
    Left  UMETA(DisplayName = "左键"),
    Right UMETA(DisplayName = "右键")
};

UCLASS()
class ALPHA_API UGA_Attack : public UAlphaGameplayAbility
{
	GENERATED_BODY()

public:
    UGA_Attack();

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

protected:
    // ========== 连招配置（在 GA_Attack 蓝图或 C++ 派生类里配）==========
    // 左键连招 Montage，顺序：[L1, L2, L3, L4, LAlt1, LAlt2]
    UPROPERTY(EditDefaultsOnly, Category = "Combo")
    TArray<TObjectPtr<UAnimMontage>> LeftComboMontages;

    // 右键连招 Montage，顺序：[R1, R2, R3, R4, RAlt1, RAlt2]
    UPROPERTY(EditDefaultsOnly, Category = "Combo")
    TArray<TObjectPtr<UAnimMontage>> RightComboMontages;



    // ========== 运行时连招状态 ==========
    int32 ComboStep = 0;                // 当前段号（0-based：基础 0~3，衍生 0~1）
    bool bInDerivedBranch = false;      // 是否已进入衍生分支
    bool bDerivedWindowOpen = false;    // 衍生窗口是否打开（第2段收招 Notify 置位）
    bool bHasBufferedInput = false;     // 是否有缓冲输入
    EAttackHand BufferedHand = EAttackHand::Left;       // 缓冲输入的手
    //bool bBufferedInDerivedWindow = false;              // 缓冲输入按下时窗口是否已开
    FTimerHandle InputListenerTimerHandle;   // 延迟注册输入监听器的定时器句柄（能力提前结束时用于清除，避免残留回调）

    bool bInComboWindow = false;        // 是否在「连招接续窗口」内（可接下一段）
    bool bComboWindowPassed = false;    // 接续窗口是否已结束（进入收招后段）
    bool bBufferWindowOpen = false;   // 缓冲窗口是否打开（接续窗口前的短窗口，收窄缓冲提前量）


     TObjectPtr<UAbilityTask_PlayMontageAndWait> CurrentMontageTask;  // 当前播放中的 Montage 任务（用于打断收招）
     TArray<TObjectPtr<UAbilityTask_WaitGameplayEvent>> WindowEventTasks;  // 当前段创建的窗口事件监听任务，AdvanceCombo 打断前统一结束，防止旧段 NotifyEnd 泄漏到新段


    // ========== 内部方法 ==========
    void PlaySection(EAttackHand Hand);                     // 播放当前段
    UAnimMontage* GetMontage(EAttackHand Hand) const;       // 按段号取 Montage
    void AdvanceCombo(EAttackHand Hand, bool bPressedInDerivedWindow); // 推进到下一段
    void SetupInputListeners();                                //设置输入监听器
    void EndCombo();                                        // 中断重置

    UFUNCTION()
    void OnSectionCompleted();                              // 一段 Montage 播完

    UFUNCTION()
    void OnAttackInput(FGameplayEventData Payload);         // 收到攻击输入

    UFUNCTION()
    void OnDerivedWindowOpened(FGameplayEventData Payload); // 衍生窗口打开

    UFUNCTION()
    void OnDerivedWindowClosed(FGameplayEventData Payload); // 衍生窗口关闭

    UFUNCTION()
    void OnComboWindowBegin(FGameplayEventData Payload);   // 接续窗口打开

    UFUNCTION()
    void OnComboWindowEnd(FGameplayEventData Payload);     // 接续窗口关闭

    UFUNCTION()
    void OnInterruptRequest(FGameplayEventData Payload);   // 收到中断请求（缓冲攻击优先）

    UFUNCTION()
    void OnBufferWindowOpen(FGameplayEventData Payload);   // 缓冲窗口打开
};
