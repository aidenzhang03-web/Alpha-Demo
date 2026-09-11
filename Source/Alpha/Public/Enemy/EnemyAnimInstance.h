#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "EnemyAnimInstance.generated.h"


/*
* 敌人动画实例：向 AnimGraph 暴露移动速度/方向等数据，供状态机选择 Idle/Walk/Run等。
*/

UCLASS()
class ALPHA_API UEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()


public:

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// 水平速度大小（cm/s），状态机据此判断 Idle/Walk/Run
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float Speed = 0.f;

	// 是否在移动
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsMoving = false;

	// 移动方向（相对角色朝向的角度，度）。用于转向/侧移动画，先留好备用
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float Direction = 0.f;
};