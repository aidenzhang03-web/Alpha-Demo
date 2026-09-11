// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerAnimInstance.h"
#include "AnimCharacterMovementLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
//Chooser相关头文件
#include "ChooserFunctionLibrary.h" 
#include "IObjectChooser.h"
#include "PoseSearch/PoseSearchDatabase.h"

#include "Combat/AlphaGameplayTags.h"
#include "AbilitySystemComponent.h"




UPlayerAnimInstance::UPlayerAnimInstance()
{
	// 初始化所有标量成员，避免读取到未定义值
	MovementSpeed = 0.0f;
	bIsMoving = false;
	bIsSprinting = false;
	bIsWalking = false;
	bIsInAir = false;
	MoveState = EMoveState::Idle;
	FullVelocity = FVector::ZeroVector;
	ModifyLinearVelScale = FVector::ZeroVector;

	//初始化移动过渡状态
	MoveTransition = EMoveTransition::None;
	bWasMoving = false;

	//初始化折返距离
	PivotDistance = 0.0f;

	//初始化转向状态
	bIsTurning = false;
	TurnDirection = ETurnDirection::None;

	//初始化玩家引用
	OwningPawn = nullptr;
	Player = nullptr;
	PlayerMovementComponent = nullptr;

	//初始化动画选择表和数据库引用
	AnimChooserTable = nullptr;
	SelectedPoseSearchDatabase = nullptr;

	//初始化武器拔出状态
	bWeaponIsDrawn = false;

	// 初始化攻击状态
	bIsAttacking = false;
}

void UPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	//获取玩家角色移动组件
	OwningPawn = TryGetPawnOwner();
	if (OwningPawn.IsValid())
	{
		Player = Cast<APlayerMaster>(OwningPawn.Get());
		if (Player)
		{
			PlayerMovementComponent = Player->GetCharacterMovement();
			bWasMoving = Player->GetIsMoving();
		}
	}
}

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	//调用OwningPawn
	OwningPawn = TryGetPawnOwner();
	if (!OwningPawn.IsValid())
	{
		Player = nullptr;
		PlayerMovementComponent = nullptr;
		return;
	}

	//获取玩家角色移动组件
	if (Player != OwningPawn.Get())
	{
		Player = Cast<APlayerMaster>(OwningPawn.Get());
		PlayerMovementComponent = Player ? Player->GetCharacterMovement() : nullptr;
	}

	// 任一引用无效则安全退出，彻底避免跨帧解引用
	if (!Player || !PlayerMovementComponent)
	{
		return;
	}

	//获取玩家速度向量
	FullVelocity = Player->GetVelocity();

	// 获取玩家速度
	MovementSpeed = FullVelocity.Size2D();

	// 从玩家角色获取移动状态
	bIsMoving = Player->GetIsMoving(); 

	bIsTurning = Player->GetIsTurning();
	TurnDirection = Player->GetTurnDirection();

	UpdateMoveState();  //必须在 EvaluateAnimChooser 之前，让本帧 Chooser 能读到状态

	UpdateMoveTransition();   // 必须在 EvaluateAnimChooser 之前，让本帧 Chooser 能读到过渡标志

	bIsInAir = PlayerMovementComponent->IsFalling();

	bWeaponIsDrawn = Player->GetWeaponIsDrawn();

	if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
	{
		bIsAttacking = ASC->HasMatchingGameplayTag(AlphaGameplayTags::State_Combo);
	}
	else
	{
		bIsAttacking = false;
	}

	//在状态属性更新完之后再评估 Chooser
	EvaluateAnimChooser();

	ModifyMotionPhysics();
	UpdatePivotDistance();
}

void UPlayerAnimInstance::ModifyMotionPhysics() //修改玩家角色物理参数
{
	switch (MoveState)
	{
	case EMoveState::Sprint: ModifyLinearVelScale = FVector(6, 6, 6); break;
	case EMoveState::Walk:   ModifyLinearVelScale = FVector(2, 2, 2); break;
	default:                 ModifyLinearVelScale = FVector(3, 3, 3); break;
	}
}

void UPlayerAnimInstance::UpdatePivotDistance()  //获取角色制动距离
{
	if (!PlayerMovementComponent)
	{
		PivotDistance = 0.0f;
		return;
	}

	FVector PivotDistanceVector;
	
	PivotDistanceVector = UAnimCharacterMovementLibrary::PredictGroundMovementPivotLocation(
		PlayerMovementComponent->GetCurrentAcceleration(),
		PlayerMovementComponent->GetLastUpdateVelocity(),
		PlayerMovementComponent->GroundFriction
	);

	PivotDistance = PivotDistanceVector.Size2D();
}

void UPlayerAnimInstance::UpdateMoveState()
{
	// 站立判定：不仅要求无移动输入，还要求实际速度已降到停稳阈值以下。
	// 松开按键后角色仍会惯性滑行减速，若只看 bIsMoving 会过早切入 Idle，导致动画跳变。
	if (!bIsMoving && MovementSpeed <= StopMoveExitSpeed)
	{
		// 停稳：无移动输入 且 实际速度已降到停稳阈值以下 → 站立
		MoveState = EMoveState::Idle;
	}
	// 有移动输入：以 PlayerMaster 的权威档位做映射
	else if (bIsMoving)
	{
		switch (Player->GetMoveSpeedState())
		{
		case EMoveSpeedState::Walk:   MoveState = EMoveState::Walk;   break;
		case EMoveSpeedState::Sprint: MoveState = EMoveState::Sprint; break;
		case EMoveSpeedState::Run:
		default:                       MoveState = EMoveState::Run;    break;
		}
	}
	// 注意：此处刻意没有 else 分支。
	// 当 !bIsMoving 且 MovementSpeed > StopMoveExitSpeed（减速滑行）时，
	// 故意不修改 MoveState，让它保持上一帧的档位，
	// 避免惯性滑行中产生「冲刺→跑步→站立」的中间态导致动画跳变。

	// 用 MoveState 反向刷新布尔标志，保证二者永远一致
	bIsSprinting = (MoveState == EMoveState::Sprint);
	bIsWalking = (MoveState == EMoveState::Walk);
}

void UPlayerAnimInstance::EvaluateAnimChooser() 
{

	//如果选择表为空，直接清空数据库引用并返回
	if (!AnimChooserTable)
	{
		SelectedPoseSearchDatabase = nullptr;
		return;
	}

	// ContextObject 传 this（AnimInstance 自身）
	UObject* Result = UChooserFunctionLibrary::EvaluateChooser(
		this,
		AnimChooserTable,
		UPoseSearchDatabase::StaticClass()
	);
	SelectedPoseSearchDatabase = Cast<UPoseSearchDatabase>(Result);
}

void UPlayerAnimInstance::UpdateMoveTransition()
{
	// 检测移动状态沿：上一帧到本帧的变化
	const bool bJustStarted = bIsMoving && !bWasMoving;  // 站立 → 移动（起步）
	const bool bJustStopped = !bIsMoving && bWasMoving;  // 移动 → 站立（停止）

	//起步 / 停止的瞬间触发过渡状态
	if (bJustStarted)
	{
		MoveTransition = EMoveTransition::StartMoving;
	}
	else if (bJustStopped)
	{
		MoveTransition = EMoveTransition::StopMoving;
	}

	// 过渡不会只持续一帧，而是持续到物理状态真正稳定后自动解除
	if (MoveTransition == EMoveTransition::StartMoving)
	{
		// 起步：实际速度超过阈值，说明已进入移动循环
		if (MovementSpeed >= StartMoveExitSpeed)
		{
			MoveTransition = EMoveTransition::None;
		}
	}
	else if (MoveTransition == EMoveTransition::StopMoving)
	{
		// 停止：实际速度降到阈值以下，说明已停稳
		if (MovementSpeed <= StopMoveExitSpeed)
		{
			MoveTransition = EMoveTransition::None;
		}
	}

	// 记录本帧移动状态，供下一帧比较
	bWasMoving = bIsMoving;
}


