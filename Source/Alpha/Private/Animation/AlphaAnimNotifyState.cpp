// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AlphaAnimNotifyState.h"
#include "Animation/AnimEventReceiver.h"

void UAlphaAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if (!MeshComp) return;
    // 统一转发给「动画事件接收器」（进入状态）
    if (IAnimEventReceiver* Receiver = Cast<IAnimEventReceiver>(MeshComp->GetOwner()))
        Receiver->HandleAnimStateBegin(StateType);
}

void UAlphaAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    if (!MeshComp) return;
    // 统一转发给「动画事件接收器」（退出状态）
    if (IAnimEventReceiver* Receiver = Cast<IAnimEventReceiver>(MeshComp->GetOwner()))
        Receiver->HandleAnimStateEnd(StateType);
}

