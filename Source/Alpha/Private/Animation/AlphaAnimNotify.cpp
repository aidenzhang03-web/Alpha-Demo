// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AlphaAnimNotify.h"
#include "Player/PlayerMaster.h"

void UAlphaAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (!MeshComp) return;
    // 通知统一转发给 PlayerMaster 的统一入口，由它分发
    if (APlayerMaster* Player = Cast<APlayerMaster>(MeshComp->GetOwner()))
    {
        Player->HandleAnimEvent(EventType);
    }
}