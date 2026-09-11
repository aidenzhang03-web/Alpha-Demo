// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AlphaAnimNotifyState.h"
#include "Player/PlayerMaster.h"

void UAlphaAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if (!MeshComp) return;
    // 统一转发给 PlayerMaster 分发（进入状态）
    if (APlayerMaster* Player = Cast<APlayerMaster>(MeshComp->GetOwner()))
    {
        Player->HandleAnimStateBegin(StateType);
    }
}

void UAlphaAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    if (!MeshComp) return;
    // 统一转发给 PlayerMaster 分发（退出状态）
    if (APlayerMaster* Player = Cast<APlayerMaster>(MeshComp->GetOwner()))
    {
        Player->HandleAnimStateEnd(StateType);
    }
}

