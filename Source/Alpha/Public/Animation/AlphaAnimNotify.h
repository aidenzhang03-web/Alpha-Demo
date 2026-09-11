// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AlphaAnimNotify.generated.h"


// 所有「逻辑类」动画事件在这里统一登记
UENUM(BlueprintType)
enum class EAnimEventType : uint8
{
    WeaponAttachHand UMETA(DisplayName = "武器拔出到手上"),
    WeaponAttachBack UMETA(DisplayName = "武器收回背部"),
    ComboInterruptAllowed  UMETA(DisplayName = "连招可以中断"),
    ComboBufferWindowOpen  UMETA(DisplayName = "连招缓冲窗口打开"),

    // 未来新增事件：在这里追加枚举值即可，不需要新建类
};


UCLASS(meta = (DisplayName = "动画事件通知"))
class ALPHA_API UAlphaAnimNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
    // 在动画上配置：这一帧触发哪个事件
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimEvent")
    EAnimEventType EventType;

    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimEvent")

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;
	
};
