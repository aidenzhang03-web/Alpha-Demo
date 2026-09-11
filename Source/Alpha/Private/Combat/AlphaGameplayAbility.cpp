// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/AlphaGameplayAbility.h"
#include "Player/PlayerMaster.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

// 便捷获取角色拥有者（能力的 Avatar Actor，即挂 ASC 的 APlayerMaster）
APlayerMaster* UAlphaGameplayAbility::GetPlayerMaster() const
{
    // GetAvatarActorFromActorInfo() 返回此能力当前作用的对象（Avatar），
    // 对角色技能而言就是 APlayerMaster 实例
    return Cast<APlayerMaster>(GetAvatarActorFromActorInfo());
}

// 通用播 Montage 封装：创建 AbilityTask，返回给调用方自行绑定回调并 ReadyForActivation
UAbilityTask_PlayMontageAndWait* UAlphaGameplayAbility::PlayMontageTask(
    UAnimMontage* Montage, float Rate, FName Section)
{
    if (!Montage) return nullptr;  // Montage 为空直接返回，避免创建无效 Task

    // CreatePlayMontageAndWaitProxy 是静态工厂，创建一个「播放并等待」的任务。
    // 注意：它不会自动开始，需调用方绑定回调后再调用 ReadyForActivation()。
    UAbilityTask_PlayMontageAndWait* Task =
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this,                      // OwningAbility：拥有此任务的能力（即当前 GA）
            TEXT("PlayMontage"),       // TaskInstanceName：任务名（调试用，可自定义）
            Montage,                   // MontageToPlay：要播放的 Montage
            Rate,                      // Rate：播放速率
            Section,                   // StartSection：从哪个 Section 开始播（NAME_None=从头播）
            true,                      // bStopWhenAbilityEnds：能力结束时是否停止播放
            1.0f           // AnimRootMotionTranslationScale：根运动位移缩放
        );                     

    return Task;
}
