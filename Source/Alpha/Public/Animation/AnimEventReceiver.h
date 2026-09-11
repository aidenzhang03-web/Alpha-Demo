#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Animation/AlphaAnimNotify.h"       // 提供 EAnimEventType
#include "Animation/AlphaAnimNotifyState.h"  // 提供 EAnimNotifyStateType
#include "AnimEventReceiver.generated.h"


/*
* 动画事件接收器（标记接口）
*/

UINTERFACE(MinimalAPI, NotBlueprintable)
class UAnimEventReceiver : public UInterface
{
	GENERATED_BODY()
};

// 动画事件接收接口：玩家 / 敌人都实现它，AnimNotify 统一往这里分发
class ALPHA_API IAnimEventReceiver
{
	GENERATED_BODY()

public:

	// 单帧动画事件（由 UAlphaAnimNotify 触发）
	virtual void HandleAnimEvent(EAnimEventType EventType) = 0;

	// 区间动画状态：进入（由 UAlphaAnimNotifyState::NotifyBegin 触发）
	virtual void HandleAnimStateBegin(EAnimNotifyStateType StateType) = 0;

	// 区间动画状态：退出（由 UAlphaAnimNotifyState::NotifyEnd 触发）
	virtual void HandleAnimStateEnd(EAnimNotifyStateType StateType) = 0;
};