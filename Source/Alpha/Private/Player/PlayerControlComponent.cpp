// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerControlComponent.h"
#include "Player/PlayerMaster.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Combat/AlphaGameplayTags.h"


// Sets default values for this component's properties
UPlayerControlComponent::UPlayerControlComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UPlayerControlComponent::BeginPlay()
{
	Super::BeginPlay();

	// 将输入映射上下文添加到本地玩家子系统
	APlayerMaster* Player = GetPlayerOwner();
	if (const APlayerController* PC = Cast<APlayerController>(Player ? Player->GetController() : nullptr))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (InputMappingContext)
			{
				Subsystem->AddMappingContext(InputMappingContext, 0);
			}
		}
	}
	
}


APlayerMaster* UPlayerControlComponent::GetPlayerOwner() const
{
	return Cast<APlayerMaster>(GetOwner());
}

//输入绑定
void UPlayerControlComponent::SetupPlayerInput(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		APlayerMaster* Player = GetPlayerOwner();
		if (!Player) return;

		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &UPlayerControlComponent::Moving);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &UPlayerControlComponent::StopMoving);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &UPlayerControlComponent::Look);

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &UPlayerControlComponent::HandleJump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, Player, &ACharacter::StopJumping);

		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &UPlayerControlComponent::Sprint);
		EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Started, this, &UPlayerControlComponent::Walk);

		EnhancedInputComponent->BindAction(WeaponToggleAction, ETriggerEvent::Started, this, &UPlayerControlComponent::WeaponToggle);
		EnhancedInputComponent->BindAction(WeaponNextAction, ETriggerEvent::Started, this, &UPlayerControlComponent::WeaponNext);
		EnhancedInputComponent->BindAction(WeaponPrevAction, ETriggerEvent::Started, this, &UPlayerControlComponent::WeaponPrev);

		EnhancedInputComponent->BindAction(AttackLeftAction, ETriggerEvent::Started, this, &UPlayerControlComponent::AttackLeft);
		EnhancedInputComponent->BindAction(AttackRightAction, ETriggerEvent::Started, this, &UPlayerControlComponent::AttackRight);
	}
}

// 输入处理逻辑：移动
void UPlayerControlComponent::Moving(const FInputActionValue& Value)
{
	//缓存角色，统一判空
	APlayerMaster* Player = GetPlayerOwner();
	if (!Player) return;

	// 缓存移动组件，避免反复调用 GetCharacterMovement() 且统一判空
	UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
	if (!Movement) return;

	const FVector2D MovementVector = Value.Get<FVector2D>();

	//输入几乎为零时不算移动，防止死区，不清空转向历史
	constexpr float MoveDeadZone = 0.1f;
	if (MovementVector.SizeSquared() < FMath::Square(MoveDeadZone))
	{

		// 这里不再清空 LastMoveInputDir / 转向状态，
		// 保证快速折返经过死区时判定链不断
		return;
	}

	//没有控制器时无法计算方向，直接返回
	if (!Player->GetController()) return;

	// 基于控制器旋转计算方向
	const FRotator ControlRotation = Player->GetController()->GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);
	const FMatrix RotMatrix = FRotationMatrix(YawRotation);

	const FVector ForwardDir = RotMatrix.GetUnitAxis(EAxis::X);
	const FVector RightDir = RotMatrix.GetUnitAxis(EAxis::Y);

	// 世界空间的输入方向（摄像机相对，经控制器 Yaw 旋转后）
	const FVector WorldInputDir =
		(ForwardDir * MovementVector.Y + RightDir * MovementVector.X).GetSafeNormal2D();

	// 记录最后移动输入方向（供攻击朝向使用）
	Player->LastInputWorldDir = WorldInputDir;

	// 连招攻击期间禁止移动；后摇可中断窗口内移动会中断连招（中断后下一帧连招结束即恢复移动）
	if (Player->IsInComboAttack())
	{
		Player->TryInterruptCombo();   // 后摇可中断窗口内，此操作会打断连招
		return;
	}

	// 非连招时，正常移动，标记移动状态
	Player->bIsMoving = true;
	Movement->BrakingDecelerationWalking = Player->MoveBrakingDeceleration;

	//产生移动
	Player->AddMovementInput(ForwardDir, MovementVector.Y);
	Player->AddMovementInput(RightDir, MovementVector.X);


	// ---- 转向触发判定  ----
	if (!Player->bIsTurning)
	{
		const FVector ActorForward = Player->GetActorForwardVector().GetSafeNormal2D();
		const float Dot = FMath::Clamp(FVector::DotProduct(ActorForward, WorldInputDir), -1.f, 1.f);
		const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));

		if (AngleDeg >= Player->TurnTriggerAngleDeg)
		{
			// 世界空间叉积定左右：输入方向在角色右侧 → 右转，左侧 → 左转
			// 注意：3D 叉积符号与 2D 本地叉积相反（Cross.Z > 0 为右转）
			const float Cross = FVector::CrossProduct(ActorForward, WorldInputDir).Z;
			Player->TurnDirection = (Cross > 0.f) ? ETurnDirection::Right : ETurnDirection::Left;
			Player->TurnTargetWorldDir = WorldInputDir; // 触发时固定目标方向，之后不再改变
			Player->TurnElapsedTime = 0.f;              // 重置超时计时
			Player->bIsTurning = true;


			if (Player->MoveSpeedState == EMoveSpeedState::Sprint)
			{
				// 冲刺折返：切回高加速度，让折返后的重新加速有爆发力
				// 否则 SprintTransitionAcceleration 会让「低速→640」的重新加速拖到 3 秒以上
				Movement->MaxAcceleration = Player->DefaultMaxAcceleration;
			}
		}
	}

}

//输入处理逻辑：停止移动
void UPlayerControlComponent::StopMoving(const FInputActionValue& Value)
{
	//缓存角色和移动组件
	APlayerMaster* Player = GetPlayerOwner();
	if (!Player) return;

	UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
	if (!Movement) return;

	//更新移动状态
	Player->bIsMoving = false;
	Movement->BrakingDecelerationWalking = Player->StopBrakingDeceleration;
	Player->LastInputWorldDir = FVector::ZeroVector;   // 松键后清空，站定攻击不再转向


	// 停止移动时：冲刺退回跑步；步行档位保留
	if (Player->MoveSpeedState == EMoveSpeedState::Sprint)
	{
		Player->MoveSpeedState = EMoveSpeedState::Run;
	}

	// 真正停止移动：重置转向状态机
	Player->bIsTurning = false;
	Player->TurnDirection = ETurnDirection::None;
	Player->TurnTargetWorldDir = FVector::ZeroVector;
	Player->TurnElapsedTime = 0.f;


	//更新速度
	Player->ModifyMaxWalkSpeed();

}

// 输入逻辑处理：跳跃（先判断是否中断连招，再正常起跳）
void UPlayerControlComponent::HandleJump(const FInputActionValue& Value)
{
	APlayerMaster* Player = GetPlayerOwner();
	if (!Player) return;

	// 连招攻击期间禁止跳跃；后摇可中断窗口内跳跃会中断连招
	if (Player->IsInComboAttack())
	{
		Player->TryInterruptCombo();  // 后摇可中断窗口内，跳跃先打断连招
		return;
	}

	Player->Jump();               // 再正常跳跃
}

// 输入处理逻辑：摄像头
void UPlayerControlComponent::Look(const FInputActionValue& Value)
{
	//缓存角色
	APlayerMaster* Player = GetPlayerOwner();
	if (!Player) return;

	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	if (Player->GetController())
	{
		Player->AddControllerYawInput(LookAxisVector.X);
		Player->AddControllerPitchInput(LookAxisVector.Y);
	}
}

//输入逻辑处理：切换冲刺
void UPlayerControlComponent::Sprint(const FInputActionValue& Value)
{
	//缓存角色
	APlayerMaster* Player = GetPlayerOwner();
	if (!Player) return;

	if (!Player->bIsMoving)
	{
		// 如果不是在移动时按下了冲刺键，则切换为慢跑状态
		Player->MoveSpeedState = EMoveSpeedState::Run;
		Player->ModifyMaxWalkSpeed();
		return;
	}

	// 记录切换前档位，用于区分 Walk→Sprint 与 Run→Sprint 的加速度
	const EMoveSpeedState PreviousState = Player->MoveSpeedState;

	// 根据触发事件类型更新冲刺状态
	Player->MoveSpeedState = (Player->MoveSpeedState == EMoveSpeedState::Sprint)
		? EMoveSpeedState::Run
		: EMoveSpeedState::Sprint;

	//更新速度，传入来源，加速度在此内部一次性定完
	Player->ModifyMaxWalkSpeed(PreviousState);

}

//输入逻辑处理：切换步行
void UPlayerControlComponent::Walk(const FInputActionValue& Value)
{
	//缓存角色
	APlayerMaster* Player = GetPlayerOwner();
	if (!Player) return;

	// 点按切换：步行 ↔ 跑步（切到步行会自然离开冲刺）
	Player->MoveSpeedState = (Player->MoveSpeedState == EMoveSpeedState::Walk)
		? EMoveSpeedState::Run
		: EMoveSpeedState::Walk;

	Player->ModifyMaxWalkSpeed();

}

//输入逻辑处理：拔出/收起武器
void UPlayerControlComponent::WeaponToggle(const FInputActionValue& Value)
{
	//缓存角色
	APlayerMaster* Player = GetPlayerOwner();
	if (!Player) return;

	if (Player->IsInComboAttack())
		return;

	if (UWeaponComponent* Weapon = Player->GetWeaponComponent())
		Weapon->ToggleWeapon();  // 内部处理 Montage + 防连点锁
}

// 输入逻辑处理：切换到下一把武器
void UPlayerControlComponent::WeaponNext(const FInputActionValue& Value)
{
	//缓存角色
	APlayerMaster* Player = GetPlayerOwner();
	if (!Player) return;

	if (UWeaponComponent* Weapon = Player->GetWeaponComponent())
		Weapon->SwitchWeaponNext();    // 内部处理
}

// 输入逻辑处理：切换到上一把武器
void UPlayerControlComponent::WeaponPrev(const FInputActionValue& Value)
{
	//缓存角色
	APlayerMaster* Player = GetPlayerOwner();
	if (!Player) return;

	if (UWeaponComponent* Weapon = Player->GetWeaponComponent())
		Weapon->SwitchWeaponPrev();    // 内部处理
}

//输入逻辑处理：左键攻击
void UPlayerControlComponent::AttackLeft(const FInputActionValue& Value)
{
	//缓存角色
	APlayerMaster* Player = GetPlayerOwner();
	if (!Player || !Player->GetWeaponComponent() || !Player->GetWeaponComponent()->IsWeaponDrawn()) 
		return;

	// 攻击开始：立即停止移动状态
	Player->bIsMoving = false;

	//缓存能力系统组件
	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	if (!ASC) return;

	//分发左键攻击事件
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		Player,
		AlphaGameplayTags::Input_Attack_Left,
		FGameplayEventData());

}

//输入逻辑处理：右键攻击
void UPlayerControlComponent::AttackRight(const FInputActionValue& Value)
{
	//缓存角色
	APlayerMaster* Player = GetPlayerOwner();
	if (!Player || !Player->GetWeaponComponent() || !Player->GetWeaponComponent()->IsWeaponDrawn()) 
		return;

	// 攻击开始：立即停止移动状态
	Player->bIsMoving = false;

	//缓存能力系统组件
	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	if (!ASC) return;

	//分发右键攻击事件
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		Player,
		AlphaGameplayTags::Input_Attack_Right,
		FGameplayEventData());

}