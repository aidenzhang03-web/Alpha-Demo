// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AlphaAnimNotifyState.generated.h"

// 所有「逻辑类」动画状态（区间型）在这里统一登记
UENUM(BlueprintType)
enum class EAnimNotifyStateType : uint8
{
	ComboDerivedWindow UMETA(DisplayName = "连招衍生窗口"),
    ComboWindow        UMETA(DisplayName = "连招接续窗口"),
    AttackHitWindow    UMETA(DisplayName = "攻击命中窗口"),

	// 未来新增状态：在这里追加枚举值即可，不需要新建类
};


UCLASS(meta = (DisplayName = "动画状态通知"))
class ALPHA_API UAlphaAnimNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:

    // 在动画上配置：这段区间属于哪个状态
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimState")
    EAnimNotifyStateType StateType = EAnimNotifyStateType::ComboDerivedWindow;

    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;
};
