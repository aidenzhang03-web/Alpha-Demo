// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "PlayerControlComponent.generated.h"

// 前置声明
class UInputMappingContext;
class UInputAction;
class UInputComponent;
class APlayerMaster;


// 玩家输入组件：负责 Enhanced Input 资产的持有、映射上下文注册、按键绑定与处理。
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALPHA_API UPlayerControlComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPlayerControlComponent();

	// 绑定输入（由 APlayerMaster::SetupPlayerInputComponent 转发调用）
	void SetupPlayerInput(UInputComponent* PlayerInputComponent);

protected:
	// Called when the game starts
	// 组件激活时注册输入映射上下文
	virtual void BeginPlay() override;
	
	// 便捷获取角色拥有者（输入处理逻辑通过它操作角色状态）
	APlayerMaster* GetPlayerOwner() const;

	// ========== 输入资产 ==========
	UPROPERTY(EditDefaultsOnly, Category = "Enhanced Input|Input Mapping Context")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Enhanced Input|Input Action")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Enhanced Input|Input Action")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Enhanced Input|Input Action")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Enhanced Input|Input Action")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "Enhanced Input|Input Action")
	TObjectPtr<UInputAction> WalkAction;

	UPROPERTY(EditDefaultsOnly, Category = "Enhanced Input|Input Action")
	TObjectPtr<UInputAction> WeaponToggleAction;  // 收拔武器

	UPROPERTY(EditDefaultsOnly, Category = "Enhanced Input|Input Action")
	TObjectPtr<UInputAction> WeaponNextAction;    // 下一把武器

	UPROPERTY(EditDefaultsOnly, Category = "Enhanced Input|Input Action")
	TObjectPtr<UInputAction> WeaponPrevAction;    // 上一把武器

	UPROPERTY(EditDefaultsOnly, Category = "Enhanced Input|Input Action")
	TObjectPtr<UInputAction> AttackLeftAction;    //左键攻击

	UPROPERTY(EditDefaultsOnly, Category = "Enhanced Input|Input Action")
	TObjectPtr<UInputAction> AttackRightAction;    //右键攻击

	// ========== 输入处理函数 ==========
	void Moving(const FInputActionValue& Value);
	void StopMoving(const FInputActionValue& Value);
	void HandleJump(const FInputActionValue& Value);   // 跳跃（含连招中断判断）
	void Look(const FInputActionValue& Value);
	void Sprint(const FInputActionValue& Value);
	void Walk(const FInputActionValue& Value);
	void WeaponToggle(const FInputActionValue& Value);
	void WeaponNext(const FInputActionValue& Value);
	void WeaponPrev(const FInputActionValue& Value);
	void AttackLeft(const FInputActionValue& Value);
	void AttackRight(const FInputActionValue& Value);

public:	
	

		
};
