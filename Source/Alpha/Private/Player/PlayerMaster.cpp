// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerMaster.h"
#include "Math/UnrealMathUtility.h"
#include "Net/UnrealNetwork.h"

// 角色移动组件
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/PlayerControlComponent.h"

// GAS
#include "AbilitySystemComponent.h"
#include "Combat/AlphaGameplayTags.h" 
#include "Combat/GA_Attack.h"
#include "Combat/AlphaAttributeComponent.h"

// UI
#include "UI/PlayerHUDWidget.h"
#include "Blueprint/UserWidget.h"


// Sets default values
APlayerMaster::APlayerMaster()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;   //需要Tick时再启用

	// ======== 初始化组件 ========
	// 创建玩家输入组件（Enhanced Input 集中管理）
	ControlsComponent = CreateDefaultSubobject<UPlayerControlComponent>(TEXT("ControlsComponent"));

	//创建玩家武器组件
	WeaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComponent"));

	// 创建弹簧臂组件
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = SpringArmLength;                 // 使用属性
	SpringArmComponent->bUsePawnControlRotation = true;
	SpringArmComponent->SetRelativeLocation(SpringArmOffset);

	// 创建摄像机组件并附加到弹簧臂
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);
	CameraComponent->SetRelativeLocation(CameraOffset);                    // 使用属性
	CameraComponent->bUsePawnControlRotation = false;



	// 初始化移动状态
	bIsMoving = false;


	if (GetCharacterMovement()) //获取角色移动组件
	{
		// 初始化默认移动速度
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed;

		//初始化最大加速度
		GetCharacterMovement()->MaxAcceleration = DefaultMaxAcceleration;

		// 初始化地面摩擦力
		GetCharacterMovement()->GroundFriction = GroundFriction;

		// 松开移动键后使用独立的刹车减速度，让惯性滑行更可控、更平滑
		GetCharacterMovement()->bUseSeparateBrakingFriction = true;
		GetCharacterMovement()->BrakingDecelerationWalking = StopBrakingDeceleration;

		// 角色朝向移动方向，而非控制器旋转方向
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = MovementRotationRate; // 旋转速度
		bUseControllerRotationYaw = false; // 角色不跟随控制器的Yaw旋转
	}


	// 创建 AbilitySystemComponent
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	// 创建玩家基础属性集（生命/法力/耐力）。ASC 会自动发现同 Actor 上的 AttributeSet，无需手动 InitStats。
	AttributeSet = CreateDefaultSubobject<UAlphaAttributeSet>(TEXT("AttributeSet"));

	// 创建属性组件（GE 应用统一入口）
	AttributeComponent = CreateDefaultSubobject<UAlphaAttributeComponent>(TEXT("AttributeComponent"));


	// ======== 联机复制配置 ========
	// 1) Actor 自身参与复制。不开这一行，整个角色对其他客户端不可见。
	bReplicates = true;

	// 2) 移动复制。UE5 中 bReplicateMovement 已私有化，必须走 Setter。
	SetReplicateMovement(true);

	// 2) ASC 参与复制。不开则 AttributeSet / GameplayTag 都不会同步，
	//    远处客户端看到的永远是初始值（血条不动）。
	AbilitySystemComponent->SetIsReplicated(true);

	// 3) 复制模式：
	//    Mixed = 属性值同步给所有人（别人能看到血条），GE 明细只同步给 Owner（省带宽）。
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

}

// Called when the game starts or when spawned
void APlayerMaster::BeginPlay()
{
	Super::BeginPlay();
	
	// 初始化 ASC 的 ActorInfo。Owner 和 Avatar 都是角色自身（单机直挂 ASC）
	// 不调用这行，AttributeSet 不会被 ASC 注册，属性监听失效。
	if(AbilitySystemComponent)
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// 角色制动配置
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bUseSeparateBrakingFriction = true;
		Move->BrakingFriction = 0.f;                       // 关掉与速度成正比的摩擦
		Move->BrakingDecelerationWalking = StopBrakingDeceleration;
	}

	// 授予连招能力（框架阶段：只授一个连招能力）
	if (AbilitySystemComponent && AttackAbilityClass)
	{
		AbilitySystemComponent->GiveAbility(
			FGameplayAbilitySpec(AttackAbilityClass, 1, INDEX_NONE, this));
	}

	//缓存武器组件，统一判空
	if(WeaponComponent)
		WeaponComponent->SpawnAndAttachWeapon();

	
}


void APlayerMaster::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 联机下服务器端此时 Controller 才就绪，重新初始化一次 ActorInfo
	if (AbilitySystemComponent)
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// 创建 HUD Widget
	if (HUDWidgetClass)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (!PC->IsLocalController()) return;   // CreateWidget 必须限定本地控制器，否则非本地 PC 上创建 Widget 会失败/告警。

			HUDWidget = CreateWidget<UPlayerHUDWidget>(PC, HUDWidgetClass);
			if (HUDWidget)
			{
				HUDWidget->AddToViewport();
				HUDWidget->InitializeHUD(this);
			}
		}
	}
}

UAbilitySystemComponent* APlayerMaster::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APlayerMaster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 档位是离散枚举、变化频率极低 → 默认复制条件即可，带宽可忽略。
	// 注意不要用 COND_OwnerOnly：远端角色需要它来选动画。
	DOREPLIFETIME(APlayerMaster, MoveSpeedState);

	DOREPLIFETIME(APlayerMaster, bIsMovingAuth);
	DOREPLIFETIME(APlayerMaster, SpeedAuth);
}

// Called every frame需要时启用
void APlayerMaster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 服务器：按真实速度刷新权威移动状态。服务器上角色跑完整的 PerformMovement
	if (HasAuthority())
	{
		bIsMovingAuth = bIsMoving;
		SpeedAuth = GetVelocity().Size2D();
	}

	UpdateTurnState(DeltaTime);   // 每帧评估转向状态机

	UpdateAttackTurn(DeltaTime); // 攻击朝向平滑转向
}


// 绑定输入
void APlayerMaster::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);


	// 输入绑定逻辑已抽离到输入组件，这里仅转发一次
	if (ControlsComponent)
	{
		ControlsComponent->SetupPlayerInput(PlayerInputComponent);
	}

}


//更新移动速度和状态
void APlayerMaster::ModifyMaxWalkSpeed(EMoveSpeedState PreviousState)
{
	//根据冲刺状态设定移动速度
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = GetCurrentWalkSpeed();

		// 按移动档位设置加速度：
		// 起步/跑步用高值避免起步抽搐，冲刺用低值让跑→冲刺过渡动画完整播放，
		// 步行用独立值匹配步行节奏
		switch (MoveSpeedState)
		{
		case EMoveSpeedState::Walk:
			Movement->MaxAcceleration = WalkMaxAcceleration;
			break;
		case EMoveSpeedState::Sprint:
			// Walk 直达 Sprint 用专用低加速度，Run→Sprint 用普通冲刺过渡加速度，
			// 让两者的未来轨迹斜率错开，避免 Walk_to_Sprint 被 Run_to_Sprint 顶掉
			Movement->MaxAcceleration = (PreviousState == EMoveSpeedState::Walk)
				? WalkToSprintAcceleration
				: SprintTransitionAcceleration;
			break;
		case EMoveSpeedState::Run:
			Movement->MaxAcceleration = RunTransitionAcceleration;
			break;
		default:
			Movement->MaxAcceleration = DefaultMaxAcceleration;
			break;
		}
	}

	// 同步耐力：冲刺档位扣耐力，其余档位延迟恢复
	if (AttributeComponent)
	{
		AttributeComponent->SetSprinting(MoveSpeedState == EMoveSpeedState::Sprint);
	}
}

//获取当前移动速度
float APlayerMaster::GetCurrentWalkSpeed() const
{
	switch (MoveSpeedState)
	{
	case EMoveSpeedState::Walk:   return WalkSpeed;
	case EMoveSpeedState::Sprint: return SprintSpeed;
	case EMoveSpeedState::Run:
	default:                      return RunSpeed;
	}
}

//每帧更新转向状态机，只负责「完成判定」
void APlayerMaster::UpdateTurnState(float DeltaTime)
{
	if (!bIsTurning)
	{
		return;   // 未在转向，无完成判定
	}

	// 累计转向时长，用于超时兜底
	TurnElapsedTime += DeltaTime;

	// 目标方向 = 触发折返时记录的世界输入方向
	const FVector TargetWorldDir = TurnTargetWorldDir.GetSafeNormal2D();

	// 角色当前朝向 vs 目标方向
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	const float Dot = FMath::Clamp(FVector::DotProduct(Forward, TargetWorldDir), -1.f, 1.f);
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));

	// 完成条件：朝向对齐到阈值以下，或超时兜底（防止 root motion 未旋转导致永久卡死）
	if (AngleDeg <= TurnCompleteAngleDeg || TurnElapsedTime >= TurnTimeoutSeconds)
	{
		bIsTurning = false;
		TurnDirection = ETurnDirection::None;
		TurnElapsedTime = 0.f;
		
	}
}

// 尝试将攻击朝向转向当前移动输入方向
void APlayerMaster::FaceAttackDirection()
{
	// 无有效移动输入（站定攻击 / 松键后攻击）→ 保持当前朝向
	if (LastInputWorldDir.IsNearlyZero())
	{
		bAttackTurning = false;
		return;
	}

	AttackTargetYaw = LastInputWorldDir.Rotation().Yaw;

	// 目标朝向与当前朝向已基本一致 → 无需转向
	const float AngleDiff = FMath::Abs(FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, AttackTargetYaw));
	if (AngleDiff < 1.f)
	{
		bAttackTurning = false;
		return;
	}

	bAttackTurning = true;
}

// 攻击朝向平滑转向更新（按固定角速度旋转到目标朝向）
void APlayerMaster::UpdateAttackTurn(float DeltaTime)
{
	if (!bAttackTurning) return;

	const float CurrentYaw = GetActorRotation().Yaw;
	// FixedTurn：从 CurrentYaw 朝 AttackTargetYaw 最多转 AttackTurnSpeed*DeltaTime 度（自动处理角度环绕）
	const float NewYaw = FMath::FixedTurn(CurrentYaw, AttackTargetYaw, AttackTurnSpeed * DeltaTime);
	SetActorRotation(FRotator(0.f, NewYaw, 0.f));

	// 到达目标朝向（角度差 < 1 度）→ 停止平滑转向
	if (FMath::Abs(FMath::FindDeltaAngleDegrees(NewYaw, AttackTargetYaw)) < 1.f)
	{
		bAttackTurning = false;
	}
}

// 尝试因移动/跳跃等操作中断连招
bool APlayerMaster::TryInterruptCombo()
{
	// 只有后摇「可中断」通知触发过才允许中断，且只消费一次
	if (!bComboInterruptAllowed) return false;
	bComboInterruptAllowed = false;

	if (AbilitySystemComponent)
	{
		// 发「中断请求」事件，由 GA_Attack 自己决策：
		// 有缓冲攻击 → 忽略中断（缓冲优先）；无缓冲 → 真正 EndCombo
		FGameplayEventData EventData;
		EventData.Instigator = this;
		EventData.Target = this;
		AbilitySystemComponent->HandleGameplayEvent(
			AlphaGameplayTags::Combo_InterruptRequest, &EventData);
	}
	return true;
}

// 是否正在连招攻击中
bool APlayerMaster::IsInComboAttack() const
{
	// 连招能力激活期间，ASC 挂载了 State.Combo 标签
	return AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(AlphaGameplayTags::State_Combo);
}

// 清除「可中断」标记
void APlayerMaster::ClearComboInterruptFlag()
{
	bComboInterruptAllowed = false;
}

// 对目标造成伤害（转发给属性组件，走 GE 应用）
void APlayerMaster::DealDamageToTarget(AActor* Target, float Amount)
{
	if (AttributeComponent)
		AttributeComponent->ApplyDamageToTarget(Target, Amount);
}

//动画通知事件触发
void APlayerMaster::HandleAnimEvent(EAnimEventType EventType)
{
	switch (EventType)
	{
		// ======== 收起/拔出武器 ========
	case EAnimEventType::WeaponAttachHand:
		if (WeaponComponent)
			WeaponComponent->AttachWeaponToHand();  // 转发拔出武器
		break;

	case EAnimEventType::WeaponAttachBack:
		if (WeaponComponent)
			WeaponComponent->AttachWeaponToBack();  // 转发收起武器
		break;


	case EAnimEventType::ComboInterruptAllowed:
		bComboInterruptAllowed = true;  //可以中断连招
		break;

	case EAnimEventType::ComboBufferWindowOpen:
		// 缓冲窗口打开：转发给 GAS，让 GA_Attack 开始接受缓冲输入
		if (AbilitySystemComponent)
		{
			FGameplayEventData EventData;
			EventData.Instigator = this;
			EventData.Target = this;
			AbilitySystemComponent->HandleGameplayEvent(
				AlphaGameplayTags::Combo_BufferWindowOpen, &EventData);
		}
		break;

	default:
		break;
	}

	
}

// 动画状态通知（区间型）：进入状态，统一分发
void APlayerMaster::HandleAnimStateBegin(EAnimNotifyStateType StateType)
{
	switch (StateType)
	{
	case EAnimNotifyStateType::ComboDerivedWindow:
	{
		// 衍生窗口打开：转发给 GAS，让 GA_Attack 的 WaitGameplayEvent 收到
		if (AbilitySystemComponent)
		{
			FGameplayEventData EventData;
			EventData.Instigator = this;
			EventData.Target = this;
			AbilitySystemComponent->HandleGameplayEvent(
				AlphaGameplayTags::Combo_DerivedWindow, &EventData);
		}
		break;
	}

	case EAnimNotifyStateType::ComboWindow:
		// 连招窗口开启
		if (AbilitySystemComponent)
		{
			FGameplayEventData EventData;
			EventData.Instigator = this;
			EventData.Target = this;
			AbilitySystemComponent->HandleGameplayEvent(
				AlphaGameplayTags::Combo_Window, &EventData);
		}
		break;

	case EAnimNotifyStateType::AttackHitWindow:
		// 攻击命中窗口打开：开启武器碰撞盒
		// 命中判定必须权威：只有服务器开 Hitbox 才能命中目标并施加 GE。
		if (!HasAuthority()) break;
		if (WeaponComponent) WeaponComponent->EnableWeaponHitbox();
		break;


	default:
		break;
	}
}

// 动画状态通知（区间型）：退出状态，统一分发
void APlayerMaster::HandleAnimStateEnd(EAnimNotifyStateType StateType)
{
	switch (StateType)
	{
	case EAnimNotifyStateType::ComboDerivedWindow:
	{
		// 衍生窗口关闭：过了此点不能再接衍生
		if (AbilitySystemComponent)
		{
			FGameplayEventData EventData;
			EventData.Instigator = this;
			EventData.Target = this;
			AbilitySystemComponent->HandleGameplayEvent(
				AlphaGameplayTags::Combo_DerivedWindowEnd, &EventData);
		}
		break;
	}

	case EAnimNotifyStateType::ComboWindow:
		// 连招窗口关闭
		if (AbilitySystemComponent)
		{
			FGameplayEventData EventData;
			EventData.Instigator = this;
			EventData.Target = this;
			AbilitySystemComponent->HandleGameplayEvent(
				AlphaGameplayTags::Combo_WindowEnd, &EventData);
		}
		break;

	case EAnimNotifyStateType::AttackHitWindow:
		// 攻击命中窗口关闭：关闭武器碰撞盒
		if (!HasAuthority()) break;
		if (WeaponComponent) WeaponComponent->DisableWeaponHitbox();
		break;

	default:
		break;
	}
}