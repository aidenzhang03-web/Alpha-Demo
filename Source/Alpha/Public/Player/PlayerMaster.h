// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

// ---- 表现层组件 ----
#include "Camera/CameraComponent.h"                        //摄像机组件
#include "GameFramework/SpringArmComponent.h"      //弹簧臂组件
#include "Weapon/WeaponComponent.h"                      //武器组件类

// ---- 动画事件链路（两个 Notify 提供枚举类型，Receiver 提供接收接口，缺一不可）----
#include "Animation/AlphaAnimNotify.h"           // 动画事件通知（提供 EAnimEventType 枚举类型）
#include "Animation/AlphaAnimNotifyState.h"   // 动画状态通知（提供 EAnimNotifyStateType 枚举类型）
#include "Animation/AnimEventReceiver.h"       // 动画事件接收器（本类实现的接口）

// ---- GAS（ASC 与 AttributeSet 是基类成员的完整类型，必须 include）----
#include "AbilitySystemInterface.h"                 // 让外部拿到 ASC     
#include "AbilitySystemComponent.h" 
#include "Combat/AlphaAttributeSet.h"

#include "PlayerMaster.generated.h"


// 前置声明（仅作指针 / 模板参数用）
class AController;                  // 控制器（PossessedBy / OnRep_Controller 参数）
class UAnimMontage;                 // 死亡蒙太奇
class UPlayerControlComponent;      // 玩家输入组件
class UGA_Attack;                   // 连招能力
class UPlayerHUDWidget;             // 玩家 HUD
class UGameplayEffect;              // 游戏效果（GE）
class UAlphaAttributeComponent;     // 属性组件（GE 应用统一入口）


/** 
 *  玩家移动档位。客户端按输入切换后经 Server_SetMoveSpeedState 上行，服务器为权威并据此刷新
 *  MaxWalkSpeed / MaxAcceleration，再把档位复制回各端供远端动画选步速。
 *  档位不只决定速度，还决定加速度曲线（见 ModifyMaxWalkSpeed）。
 */
UENUM(BlueprintType)
enum class EMoveSpeedState : uint8
{
	Walk     UMETA(DisplayName = "步行"),      // 步行
	Run      UMETA(DisplayName = "跑步"),      // 跑步（默认）
	Sprint   UMETA(DisplayName = "冲刺")       // 冲刺
};

/** 
 *  折返转向方向。仅表示「往哪边拐」：左右由触发瞬间输入方向与当前朝向的叉积决定
 *  （PlayerControlComponent.cpp:149），UpdateTurnState 在对齐完成或超时后置回 None。
 *  不复制，各端本地计算。
 */
UENUM(BlueprintType)
enum class ETurnDirection : uint8
{
	None    UMETA(DisplayName = "无转向"),
	Left    UMETA(DisplayName = "左转"),
	Right   UMETA(DisplayName = "右转")
};


/** 死亡状态变化广播（true = 已死亡）。详细说明见 OnDeadStateChanged。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeadStateChangedSignature, bool, bIsDead);


/** 
 *  玩家角色主类：持有并复制跨端状态、把请求转发给功能组件、实现规则逻辑与动画事件分发。
 *
 *  网络分工：判定（命中盒、伤害 GE）只在服务器执行；表现（碰撞、输入、蒙太奇）由
 *  Multi_Die / Multi_Revive 在各端本地执行；档位与冲刺折返加速度由客户端上行，服务器据此
 *  刷新 MaxWalkSpeed / MaxAcceleration。需复制的成员一律不用 COND_OwnerOnly——远端要靠它选动画。
 *  远端动画只读 GetIsMovingAuth / GetSpeedAuth，不要用位置差分或本地输入状态推断。 
 */
UCLASS()
class ALPHA_API APlayerMaster : public ACharacter, public IAbilitySystemInterface, public IAnimEventReceiver
{
	GENERATED_BODY()

	// 允许输入组件直接读写本类的 protected 成员（LastInputWorldDir / bIsMoving / TurnDirection 等都由它写入）
	friend class UPlayerControlComponent;

public:
	// ======== 构造与引擎回调 ========

	/** 构造：创建各功能组件，配置移动参数与网络复制。 */
	APlayerMaster();

	/** 每帧：服务器端刷新权威移动状态（bIsMovingAuth / SpeedAuth），所有端更新转向状态机。 */
	virtual void Tick(float DeltaTime) override;

	/** 把输入绑定转发给 UPlayerControlComponent，本类不直接绑定。 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/**
	 *  服务器侧被控制器占有时调用（重新初始化 ASC 的 ActorInfo 并创建 HUD）。
	 *  客户端走的是 OnRep_Controller，两者最终都汇入 TryCreateHUD。
	 */
	virtual void PossessedBy(AController* NewController) override;

	/** Controller 复制到达（客户端）时创建 HUD；PossessedBy 是服务器专属，客户端不执行。 */
	virtual void OnRep_Controller() override;



	// ======== 接口实现 ========

	/** IAbilitySystemInterface：对外暴露 ASC。 */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** 注册复制属性：移动档位、权威移动状态、死亡状态。 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 动画事件统一分发入口，由 UAnimNotify_AnimEvent 在特定帧调用；区间型状态走 HandleAnimStateBegin / End。 */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void HandleAnimEvent(EAnimEventType EventType) override;

	/** 区间状态「进入」：由 UAnimNotifyState_AnimEvent 调用。
	 *  连招 / 衍生窗口 → 转发对应 GAS 事件；命中窗口 → 开武器命中盒（仅服务器）。 */
	void HandleAnimStateBegin(EAnimNotifyStateType StateType) override;

	/** 区间状态「退出」：与 Begin 对称。关窗口事件 / 关命中盒（仅服务器）。 */
	void HandleAnimStateEnd(EAnimNotifyStateType StateType) override;



	// ======== 移动状态查询 ========

	/** 本地输入层：是否正在接收移动输入。不复制，仅本地玩家有意义；松键立即 false。 */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	bool GetIsMoving() const { return bIsMoving; }

	/** 服务器权威：角色是否真的在移动。服务器每帧刷新并复制，是远端角色唯一可信的依据。 */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	bool GetIsMovingAuth() const { return bIsMovingAuth; }

	/** 服务器权威水平速度（cm/s）。远端动画用；不要改用 GetVelocity()——平滑结束时它会断崖归零。 */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	float GetSpeedAuth() const { return SpeedAuth; }

	/** 当前移动档位（复制字段，两端可读）。 */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	EMoveSpeedState GetMoveSpeedState() const { return MoveSpeedState; }



	// ======== 转向查询 ========

	/** 当前折返转向方向。不复制，未在转向中时为 None。 */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	ETurnDirection GetTurnDirection() const { return TurnDirection; }

	/** 是否正处于折返转向过程中。不复制。 */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	bool GetIsTurning() const { return bIsTurning; }



	// ======== 武器 Getter ========

	/** 武器组件指针，可能为 nullptr。 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	UWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

	/** 武器是否已拔出。内部已判空，组件缺失时返回 false。 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool GetWeaponIsDrawn() const { return WeaponComponent && WeaponComponent->IsWeaponDrawn(); }



	// ======== 属性集 Getter ========
	// 统一在属性集缺失时返回 0，调用处不必判空。

	/** 当前生命值。 */
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetHealth() const { return AttributeSet ? AttributeSet->GetHealth() : 0.f; }

	/** 生命值上限。 */
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMaxHealth() const { return AttributeSet ? AttributeSet->GetMaxHealth() : 0.f; }

	/** 当前法力值。 */
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMana() const { return AttributeSet ? AttributeSet->GetMana() : 0.f; }

	/** 法力值上限。 */
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMaxMana() const { return AttributeSet ? AttributeSet->GetMaxMana() : 0.f; }

	/** 当前耐力值。冲刺档位或者在其他情况下会扣减，非冲刺档位延迟恢复。 */
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetStamina() const { return AttributeSet ? AttributeSet->GetStamina() : 0.f; }

	/** 耐力值上限。 */
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMaxStamina() const { return AttributeSet ? AttributeSet->GetMaxStamina() : 0.f; }



	// ======== 死亡 ========

	/** 是否已死亡。供动画层与外部系统查询。 */
	UFUNCTION(BlueprintCallable, Category = "Death")
	bool IsDead() const { return bDead; }

	/** 死亡：服务器权威入口。只做判定与广播，不做表现。 */
	void Die();

	/** 死亡表现：每个端各自执行（动画 / 碰撞 / 输入都是本地状态，不参与复制）。 */
	UFUNCTION(NetMulticast, Reliable)
	void Multi_Die();

	/**
	 * 死亡状态变化广播。
	 * 由 Multi_Die / Multi_Revive 在所有端各自触发，HUD 据此显示或隐藏个人死亡面板。
	 * 刻意不用 RepNotify：Multi_Die 里已本地赋值 bDead，属性复制到达时新旧值相同，
	 * OnRep 不会触发，客户端 HUD 就收不到通知。
	 */
	UPROPERTY(BlueprintAssignable, Category = "Death")
	FOnDeadStateChangedSignature OnDeadStateChanged;



	// ======== 复活 ========

	/** 请求复活（客户端可调）。内部转发 Server RPC，调用处不必判 HasAuthority。 */
	UFUNCTION(BlueprintCallable, Category = "Death")
	void RequestRevive();

	/** 复活：服务器权威入口。重置状态 + 回血 + 解除失败锁定，不做表现。 */
	void Revive();

	/** 复活表现：每个端各自执行（碰撞 / 输入 / 蒙太奇都是本地状态）。 */
	UFUNCTION(NetMulticast, Reliable)
	void Multi_Revive();



	// ======== 连招 ========

	/**
	 *  尝试因移动 / 跳跃中断连招。仅在后摇「可中断」通知触发过后生效，且只消费一次。
	 *  内部发 Combo_InterruptRequest 事件，由 GA_Attack 决策：有缓冲攻击则忽略，无缓冲才真正断。
	 */
	UFUNCTION(BlueprintCallable, Category = "Combo")
	bool TryInterruptCombo();

	/** 是否正在连招攻击中。按 ASC 上的 State.Combo 标签判定。 */
	bool IsInComboAttack() const;

	/** 把攻击朝向转到当前移动输入方向。每段连击开始时调用；无有效输入则保持当前朝向。 */
	void FaceAttackDirection();

	/** 清除「可中断」标记。连招每段开始 / 结束时调用，防止残留到下一段。 */
	void ClearComboInterruptFlag();

	/** 对目标造成伤害（转发给属性组件走 GE），供武器命中时调用。 */
	void DealDamageToTarget(AActor* Target, float Amount);



protected:
	/** 初始化：注册 ASC 的 ActorInfo、配置制动参数、授予连招能力、生成武器、绑定生命归零回调。 */
	virtual void BeginPlay() override;



	// ======== 移动状态 ========
	
	/** 
	 *  当前移动档位。服务器权威并复制到各端，供远端角色的动画层读取档位。
	 *  初始为跑步，之后只由 Server_SetMoveSpeedState 改写。 
	 */
	UPROPERTY(Replicated)
	EMoveSpeedState MoveSpeedState = EMoveSpeedState::Run;

	// 本地输入层：是否正在接收移动输入。由 UPlayerControlComponent 在输入回调里改写，不复制
	bool bIsMoving;

	/** 
	 *  服务器权威：角色是否真的在移动。服务器每帧按当前加速度刷新，复制给所有客户端。
	 *  远端角色（SimulatedProxy）的动画必须读它 —— 远端既没有本地输入状态，
	 *  速度字段在平滑结束后也不可信，只能靠服务器给结论。
	 */
	UPROPERTY(Replicated)
	bool bIsMovingAuth = false;

	/** 
	 *  服务器权威水平速度（cm/s），复制给客户端供远端动画使用。
	 *  远端不用位置差分推断：位置差分在网络平滑结束时断崖归零，
	 *  会使停止过渡窗口被截短（走档实测仅 48ms，本地 82ms），过渡动画来不及播。 
	 */
	UPROPERTY(Replicated)
	float SpeedAuth = 0.f;



	// ======== 死亡状态 ========

	/** 属性集「生命归零」回调。动态委托要求必须是 UFUNCTION。 */
	UFUNCTION()
	void HandleOutOfHealth();

	/** 服务器实际执行复活。 */
	UFUNCTION(Server, Reliable)
	void Server_RequestRevive();

	/** 死亡标志位。服务器置位后复制到客户端，客户端据此驱动动画与输入锁。 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Death")
	bool bDead = false;

	/** 死亡蒙太奇（在 BP_PlayerMaster 里指定）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Death")
	TObjectPtr<UAnimMontage> DeathMontage;



	// ======== 攻击朝向（每段连击开始时按输入方向平滑转向）========

	// 最后有效的移动输入世界方向（摄像机相对）。由输入组件写入，松键时清零
	FVector LastInputWorldDir = FVector::ZeroVector;

	// 攻击朝向平滑转向是否进行中
	bool bAttackTurning = false;

	// 攻击目标朝向 Yaw
	float AttackTargetYaw = 0.f;

	/** 攻击转向速度（度/秒）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0"))
	float AttackTurnSpeed = 720.f;



	// ======== 组件声明 ========

	/** 摄像机组件，挂在弹簧臂末端。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> CameraComponent;

	/** 弹簧臂组件，承载摄像机并跟随控制器旋转。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArmComponent;

	/** 玩家输入组件（Enhanced Input 集中管理）。本类的输入回调与移动输入状态都由它写入。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UPlayerControlComponent> ControlsComponent;

	/** 武器组件，负责武器生成、附着与命中盒开关。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponComponent> WeaponComponent;

	/** 能力系统组件（GAS 入口）。已开启复制，ReplicationMode 为 Mixed。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	/** 玩家基础属性集（生命 / 法力 / 耐力）。ASC 会自动发现同 Actor 上的它，无需手动 InitStats。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAlphaAttributeSet> AttributeSet;

	/** 属性组件，GE 应用的统一入口（扣血 / 回血 / 耐力变更都经它）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAlphaAttributeComponent> AttributeComponent;



	// ======== 移动速度配置 ========

	/** 跑步（默认档位）速度，cm/s。 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Speed", meta = (ClampMin = "0"))
	float RunSpeed = 435.f;

	/** 冲刺档位速度，cm/s。 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Speed", meta = (ClampMin = "0"))
	float SprintSpeed = 685.f;

	/** 步行档位速度，cm/s。 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Speed", meta = (ClampMin = "0"))
	float WalkSpeed = 200.f;



	// ======== 移动档位与速度 ========

	/**
	 *  按当前档位刷新 MaxWalkSpeed 与 MaxAcceleration，并同步耐力消耗状态。
	 *
	 *  @param PreviousState 切换前的档位，用于区分 Walk→Sprint 与 Run→Sprint；
	 *         默认值 Run 仅作「无特殊来源」哨兵——只有当前是 Sprint 且来源是 Walk 时
	 *         才走 WalkToSprintAcceleration 分支。
	 */
	void ModifyMaxWalkSpeed(EMoveSpeedState PreviousState = EMoveSpeedState::Run);

	/**
	 *  客户端上报移动档位切换。服务器为权威：设置权威档位并按新档位刷新
	 *  MaxWalkSpeed / MaxAcceleration。重复上报同一档位会被忽略（幂等）。
	 */
	UFUNCTION(Server, Reliable)
	void Server_SetMoveSpeedState(EMoveSpeedState NewState);

	/**
	 *  客户端上报「冲刺折返」触发。服务器为权威：把加速度提升到 DefaultMaxAcceleration，
	 *  让折返后的重新加速节奏与客户端预测保持一致，避免权威轨迹落后触发位置校正。
	 */
	UFUNCTION(Server, Reliable)
	void Server_ApplyPivotAcceleration();

	/** 按当前档位返回配置速度：Walk / Sprint 分别取 WalkSpeed / SprintSpeed，其余取 RunSpeed。 */
	float GetCurrentWalkSpeed() const;



	// ======== 移动参数（加速度 / 摩擦）========
	// 不同档位、不同切换路径用不同加速度，目的是让起步 / 过渡动画的节奏与实际位移对得上。
	// 按档位与来源档位的分支逻辑在 ModifyMaxWalkSpeed 里。
	
	/** 默认加速度。冲刺折返时由 Server_ApplyPivotAcceleration 提升到这个值。 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0"))
	float DefaultMaxAcceleration = 800.f;

	/** 步行加速度：步行档位专用，匹配步行起步 / 循环动画的加速节奏。 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0"))
	float WalkMaxAcceleration = 300.f;
	
	/** 跑步加速度：跑步档位专用，匹配跑步起步 / 循环动画的加速节奏。 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0"))
	float RunTransitionAcceleration = 500.f;

	/** 冲刺过渡加速度：跑→冲刺过渡专用。取值较低，让过渡动画完整播放（过高会跳过过渡动画低速段）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0"))
	float SprintTransitionAcceleration = 350.f;

	/** 
	 *  行走→冲刺过渡专用加速度：匹配 Walk_to_Sprint 过渡动画的 root motion 加速节奏。
	 *  与 SprintTransitionAcceleration 分开，是为了让两条路径的未来轨迹斜率错开，
	 *  避免 Walk_to_Sprint 被 Run_to_Sprint 顶掉。 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0"))
	float WalkToSprintAcceleration = 200.f;

	/** 地面摩擦力。 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0"))
	float GroundFriction = 8.f;

	/** 移动中刹车的减速度（转向 / 慢速收束），cm/s²。 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Inertia", meta = (ClampMin = "0"))
	float MoveBrakingDeceleration = 300.f;

	/** 松键停止的刹车减速度，cm/s²。与移动中刹车分开，让惯性滑行更可控、更平滑。 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Inertia", meta = (ClampMin = "0"))
	float StopBrakingDeceleration = 1200.f;

	/** 朝向移动方向的旋转速率。配合 bOrientRotationToMovement 使用。 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	FRotator MovementRotationRate = FRotator(0.f, 540.f, 0.f);



	// ======== 摄像机/弹簧臂 ========

	/** 弹簧臂长度，cm。 */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float SpringArmLength = 350.f;

	/** 弹簧臂相对角色的挂载偏移。 */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FVector SpringArmOffset = FVector(0.f, 0.f, 50.f);

	/** 摄像机相对弹簧臂末端的偏移。 */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FVector CameraOffset = FVector(0.f, 70.f, 20.f);



	// ======== 转向相关 ========
	// 折返是「输入方向与当前朝向夹角过大」时的过渡处理：
	// 输入回调里按夹角判定是否触发（阈值见 TurnTriggerAngleDeg），
	// 本类 Tick 里的 UpdateTurnState 只负责完成判定。

	/** 
	 *  当前折返方向。触发瞬间由角色朝向与输入方向的叉积定左右
	 *  （PlayerControlComponent.cpp:149），对齐完成或超时后置回 None。
	 *  不复制，各端本地计算。 
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Turn")
	ETurnDirection TurnDirection = ETurnDirection::None;

	// 折返目标方向。触发时固定为当时的世界输入方向，之后不再改变，仅供完成判定用
	FVector TurnTargetWorldDir = FVector::ZeroVector;

	// 折返是否进行中
	bool bIsTurning = false;

	// 折返已持续时长，用于超时兜底
	float TurnElapsedTime = 0.f;

	/** 触发阈值（度）：角色当前朝向与世界输入方向的夹角达到此值即进入折返。 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Turn", meta = (ClampMin = "0", ClampMax = "180"))
	float TurnTriggerAngleDeg = 75.f;

	/** 完成阈值（度）：朝向与目标的夹角降到此值以下视为折返完成。 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Turn", meta = (ClampMin = "0", ClampMax = "180"))
	float TurnCompleteAngleDeg = 15.f;

	/** 超时兜底（秒）：超过则强制退出折返，防止 root motion 未旋转导致永久卡住。 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Turn", meta = (ClampMin = "0"))
	float TurnTimeoutSeconds = 1.5f;

	/** 每帧评估折返是否完成：朝向对齐到 TurnCompleteAngleDeg 以下，或超过 TurnTimeoutSeconds。 */
	void UpdateTurnState(float DeltaTime);

	/** 攻击朝向平滑转向更新：按 AttackTurnSpeed 的角速度转向 AttackTargetYaw。 */
	void UpdateAttackTurn(float DeltaTime);



	// ======== 玩家HUD ========

	/** HUD 控件类，蓝图里指到 WBP_PlayerHUD。 */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UPlayerHUDWidget> HUDWidgetClass;

	/** HUD 实例引用。仅在本地控制器上创建，用于后续显隐与状态更新。 */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UPlayerHUDWidget> HUDWidget;

	/** 创建 HUD 并绑定。服务器与客户端都会调用，内部幂等。 */
	void TryCreateHUD();



	// ======== 连招状态 ========

	/** 连招能力类，在 BP_PlayerMaster 里指定；BeginPlay 时由 ASC 授予。 */
	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	TSubclassOf<UGA_Attack> AttackAbilityClass;

	// 连招后摇的「可中断」标记：动画通知置位，TryInterruptCombo 消费后立即清零
	bool bComboInterruptAllowed = false;

	/**
	 *  以自身同时作为 Instigator / Target 向 ASC 发送动画事件。
	 *  ASC 缺失时静默忽略（与各调用点原有判空行为一致）。
	 *  每次独立构造 EventData：GAS 会把指针交给能力，复用同一对象有悬垂风险。
	 */
	void SendGameplayEvent(const FGameplayTag& EventTag) const;
};
