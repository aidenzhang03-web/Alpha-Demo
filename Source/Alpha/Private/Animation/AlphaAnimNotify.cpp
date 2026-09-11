// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AlphaAnimNotify.h"
#include "Animation/AnimEventReceiver.h" 

void UAlphaAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (!MeshComp) return;
    // 统一转发给「动画事件接收器」（玩家/敌人通用）
    if (IAnimEventReceiver* Receiver = Cast<IAnimEventReceiver>(MeshComp->GetOwner()))
        Receiver->HandleAnimEvent(EventType);
}