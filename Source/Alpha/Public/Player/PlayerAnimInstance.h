// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/PlayerMaster.h"
#include "PlayerAnimInstance.generated.h"

class UChooserTable;
class UPoseSearchDatabase;


UENUM(BlueprintType)
enum class EMoveState : uint8
{
	Idle     UMETA(DisplayName = "站立"),
	Walk   UMETA(DisplayName = "步行"),
	Run     UMETA(DisplayName = "跑步"),
	Sprint  UMETA(DisplayName = "冲刺")
};

// 站立↔移动过渡枚举
UENUM(BlueprintType)
enum class EMoveTransition : uint8
{
	None         UMETA(DisplayName = "无过渡"),
	StartMoving  UMETA(DisplayName = "站立→移动"),
	StopMoving   UMETA(DisplayName = "移动→站立")
};

UCLASS()
class ALPHA_API UPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPlayerAnimInstance();

	// 新增动画变量
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float MovementSpeed; //移动速度

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	EMoveState MoveState;//移动状态

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsMoving; //是否移动

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsSprinting; //是否冲刺

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsWalking; //是否步行

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsInAir; //是否在空中

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	FVector FullVelocity; // 完整速度向量

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float PivotDistance; //折返运动时制动的距离

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	FVector ModifyLinearVelScale; //组件线性速度标度

	// 转向状态（动画层副本，由移动层同步）
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	ETurnDirection TurnDirection;   // 转向方向

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsTurning;              // 是否正在转向

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bWeaponIsDrawn;  //武器是否拔出来

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsAttacking;   // 是否正在攻击（连招进行中，由 GAS 的 State.Combo Tag 驱动）

	// Chooser 表资产，可在蓝图中指定
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UChooserTable> AnimChooserTable;

	// Chooser 评估选中的 PoseSearchDatabase（供 Motion Matching 使用）
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UPoseSearchDatabase> SelectedPoseSearchDatabase;

	// 站立↔移动过渡状态（供 Chooser 过滤与 AnimGraph 打断 continuing pose 使用）
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	EMoveTransition MoveTransition;

	// 起步过渡退出阈值：速度超过该值即认为已进入移动循环
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Transition", meta = (ClampMin = "0"))
	float StartMoveExitSpeed = 150.f;

	// 停止过渡退出阈值：速度降到该值以下即认为已停稳
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Transition", meta = (ClampMin = "0"))
	float StopMoveExitSpeed = 50.f;

protected:
	void ModifyMotionPhysics(); //修改移动时角色物理参数
	void UpdatePivotDistance(); //计算折返时急停的距离
	
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	void EvaluateAnimChooser();  // 评估 Chooser 表，更新 SelectedPoseSearchDatabase
	void UpdateMoveState();   // 从 Player 的权威状态映射出动画层的 MoveState
	void UpdateMoveTransition();  // 检测站立↔移动过渡沿并更新过渡状态

private:

	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> OwningPawn;

	//添加玩家缓存指针
	UPROPERTY(Transient)
	APlayerMaster* Player;

	//添加玩家移动组件指针
	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> PlayerMovementComponent;

	// 上一帧是否在移动（用于检测过渡沿）
	bool bWasMoving = false;
	
};
