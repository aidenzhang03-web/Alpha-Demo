// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h" //摄像机组件
#include "GameFramework/SpringArmComponent.h" //弹簧臂组件
#include "Weapon/WeaponComponent.h" //武器组件类
#include "Animation/AlphaAnimNotify.h" // 动画事件通知（提供 EAnimEventType 枚举类型）
#include "Animation/AlphaAnimNotifyState.h" // 动画状态通知（提供 EAnimNotifyStateType 枚举类型）
#include "Animation/AnimEventReceiver.h"     // 动画事件接收器

#include "AbilitySystemInterface.h"      
#include "AbilitySystemComponent.h" 
#include "Combat/AlphaAttributeSet.h"

#include "PlayerMaster.generated.h"

// 前置声明
class AController;
class UAnimMontage;
class UPlayerControlComponent;
class UWeaponComponent;
class UGA_Attack;
class UPlayerHUDWidget;
class UGameplayEffect;
class UAlphaAttributeComponent;

//移动状态枚举
UENUM(BlueprintType)
enum class EMoveSpeedState : uint8
{
	Walk     UMETA(DisplayName = "步行"),      // 步行
	Run      UMETA(DisplayName = "跑步"),      // 跑步（默认）
	Sprint   UMETA(DisplayName = "冲刺")       // 冲刺
};

//转向枚举
UENUM(BlueprintType)
enum class ETurnDirection : uint8
{
	None    UMETA(DisplayName = "无转向"),
	Left    UMETA(DisplayName = "左转"),
	Right   UMETA(DisplayName = "右转")
};



UCLASS()
class ALPHA_API APlayerMaster : public ACharacter, public IAbilitySystemInterface, public IAnimEventReceiver
{
	GENERATED_BODY()

	// 允许输入组件直接访问本类的 protected 成员，避免把内部状态暴露成 public 破坏封装
	friend class UPlayerControlComponent;

public:
	// Sets default values for this character's properties
	APlayerMaster();

	// 重写IAbilitySystemInterface：让外部拿到 ASC
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** 属性复制注册：把移动档位同步给客户端。 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Animation")
	bool GetIsMoving() const { return bIsMoving; }

	UFUNCTION(BlueprintCallable, Category = "Animation")
	EMoveSpeedState GetMoveSpeedState() const { return MoveSpeedState; }

	UFUNCTION(BlueprintCallable, Category = "Animation")
	ETurnDirection GetTurnDirection() const { return TurnDirection; }

	UFUNCTION(BlueprintCallable, Category = "Animation")
	bool GetIsTurning() const { return bIsTurning; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	UWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool GetWeaponIsDrawn() const { return WeaponComponent && WeaponComponent->IsWeaponDrawn(); }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void HandleAnimEvent(EAnimEventType EventType) override;  // 动画事件统一分发入口（由 UAnimNotify_AnimEvent 在动画特定帧调用）


	// ======== 角色属性集Getter ========
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetHealth() const { return AttributeSet ? AttributeSet->GetHealth() : 0.f; }

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMaxHealth() const { return AttributeSet ? AttributeSet->GetMaxHealth() : 0.f; }

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMana() const { return AttributeSet ? AttributeSet->GetMana() : 0.f; }

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMaxMana() const { return AttributeSet ? AttributeSet->GetMaxMana() : 0.f; }

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetStamina() const { return AttributeSet ? AttributeSet->GetStamina() : 0.f; }

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMaxStamina() const { return AttributeSet ? AttributeSet->GetMaxStamina() : 0.f; }


	// 尝试因移动/跳跃等操作中断连招。仅在后摇「可中断」通知触发后生效，且只消费一次。
	UFUNCTION(BlueprintCallable, Category = "Combo")
	bool TryInterruptCombo();

	bool IsInComboAttack() const;  // 是否正在连招攻击中

	void FaceAttackDirection();  // 尝试将攻击朝向转向当前移动输入方向（每段连击开始时调用；无有效输入则保持当前朝向）

	// 清除「可中断」标记（连招每段开始/结束时调用，防止残留到下一段）
	void ClearComboInterruptFlag();

	// 对目标造成伤害（内部转发给属性组件走 GE，供武器命中时调用）
	void DealDamageToTarget(AActor* Target, float Amount);

	// 动画状态统一分发入口（区间型状态：进入/退出，由 UAnimNotifyState_AnimEvent 调用）
	void HandleAnimStateBegin(EAnimNotifyStateType StateType) override;
	void HandleAnimStateEnd(EAnimNotifyStateType StateType) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 移动档位（权威状态）。复制到所有客户端，供远端角色的动画层读取档位
	UPROPERTY(Replicated)
	EMoveSpeedState MoveSpeedState = EMoveSpeedState::Run;    // 默认为跑步状态

	// 移动状态变量
	bool bIsMoving;  //是否正在移动

	bool bComboInterruptAllowed = false;   //是否能够中断连招



	// ========== 攻击朝向（每段连击开始时按输入方向平滑转向）==========
	FVector LastInputWorldDir = FVector::ZeroVector;  // 最后有效的移动输入世界方向（摄像机相对）
	bool bAttackTurning = false;                       // 攻击朝向平滑转向进行中
	float AttackTargetYaw = 0.f;                       // 攻击目标朝向 Yaw

	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0"))
	float AttackTurnSpeed = 720.f;                     // 攻击转向速度（度/秒）



	// ========== 组件声明 ==========
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> CameraComponent; // 摄像机组件

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArmComponent; // 弹簧臂组件

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UPlayerControlComponent> ControlsComponent; // 玩家输入组件（Enhanced Input 集中管理）

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponComponent> WeaponComponent;  //玩家武器组件

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;  //能力系统组件

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAlphaAttributeSet> AttributeSet;  // 玩家基础属性集

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAlphaAttributeComponent> AttributeComponent;  // 属性组件（GE 应用统一入口）



	// ========== 移动速度配置 ==========
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Speed", meta = (ClampMin = "0"))
	float RunSpeed = 435.f;      // 跑步（默认）速度

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Speed", meta = (ClampMin = "0"))
	float SprintSpeed = 685.f;   // 冲刺速度

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Speed", meta = (ClampMin = "0"))
	float WalkSpeed = 200.f;     // 步行速度



	// ========== 移动参数 ==========
	// 默认加速度
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0"))
	float DefaultMaxAcceleration = 800.f;

	// 步行加速度：步行档位专用，匹配步行起步/循环动画的加速节奏
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0"))
	float WalkMaxAcceleration = 300.f;
	
	// 跑步加速度：跑步档位专用，匹配跑步起步 / 循环动画的加速节奏
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0"))
	float RunTransitionAcceleration = 500.f;

	// 冲刺过渡加速度：跑→冲刺过渡专用，较低让过渡动画完整播放（过高会跳过过渡动画低速段）
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0"))
	float SprintTransitionAcceleration = 350.f;

	// 行走→冲刺过渡专用加速度：匹配 Walk_to_Sprint 过渡动画的 root motion 加速节奏。
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0"))
	float WalkToSprintAcceleration = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0"))
	float GroundFriction = 8.f;             // 地面摩擦力

	// 移动中刹车（转向/慢速收束）
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Inertia", meta = (ClampMin = "0"))
	float MoveBrakingDeceleration = 300.f;

	// 松键停止刹车
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Inertia", meta = (ClampMin = "0"))
	float StopBrakingDeceleration = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	FRotator MovementRotationRate = FRotator(0.f, 540.f, 0.f); // 朝向移动方向的旋转速率



	// ========== 摄像机/弹簧臂 ==========
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float SpringArmLength = 350.f;          // 弹簧臂长度

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FVector SpringArmOffset = FVector(0.f, 0.f, 50.f);   // 弹簧臂相对位置

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FVector CameraOffset = FVector(0.f, 70.f, 20.f);     // 摄像机相对位置


	// ========== 转向相关 ==========
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Turn")
	ETurnDirection TurnDirection = ETurnDirection::None; // 权威源

	FVector TurnTargetWorldDir = FVector::ZeroVector; // 折返目标方向（触发时固定，仅供完成判定）
	bool bIsTurning = false;      // 是否正在转向，默认为false

	float TurnElapsedTime = 0.f;  // 转向已持续时长（用于超时兜底）

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Turn", meta = (ClampMin = "0", ClampMax = "180"))
	float TurnTriggerAngleDeg = 75.f;   // 触发阈值

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Turn", meta = (ClampMin = "0", ClampMax = "180"))
	float TurnCompleteAngleDeg = 15.f;  // 完成阈值

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Turn", meta = (ClampMin = "0"))
	float TurnTimeoutSeconds = 1.5f;   // 转向超时兜底（秒），超过则强制退出

	void UpdateTurnState(float DeltaTime); // 更新转向状态（带 DeltaTime 以支持超时计时）

	void UpdateAttackTurn(float DeltaTime);   // 攻击朝向平滑转向更新



	// ======== 玩家HUD ========
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UPlayerHUDWidget> HUDWidgetClass;   // 蓝图里指到 WBP_PlayerHUD

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UPlayerHUDWidget> HUDWidget;          // 保存引用，便于后续显示/隐藏


	// ========== 连招能力 ==========
	// 连招能力类（在蓝图 BP_PlayerMaster 里指定，或这里用 TSubclassOf 默认值）
	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	TSubclassOf<UGA_Attack> AttackAbilityClass;


	// 修改移动速度与加速度。
   // PreviousState：切换前的档位，用于区分 Walk→Sprint（专用低加速度）与 Run→Sprint。
   // 默认值 Run 仅作「无特殊来源」哨兵——只有当前是 Sprint 且来源是 Walk 时才走特殊分支。
	void ModifyMaxWalkSpeed(EMoveSpeedState PreviousState = EMoveSpeedState::Run); 

	float GetCurrentWalkSpeed() const;//获取当前移动速度



public:	
	// Called every frame需要时启用
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void PossessedBy(AController* NewController) override;
};
