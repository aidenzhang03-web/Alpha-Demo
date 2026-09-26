// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AlphaAttributeSet.generated.h"

/**
 * 玩家基础属性集：生命值 / 法力值 / 耐力值（各含最大值）。
 * 属性数据存储在这里，但「修改」必须通过 GameplayEffect，不要直接调用 Setter。
 * 
 * 联机说明：属性值通过 GetLifetimeReplicatedProps 同步到所有客户端；
 * 服务器为权威端（PostGameplayEffectExecute 只在服务器跑），
 * 客户端靠 OnRep_* 更新本地缓存并广播变化（血条监听的就是这个事件）。
 */


 // 生命归零委托：标准 GAS 用法下 GE 由服务器应用，
 // 所以 PostGameplayEffectExecute 里的广播天然只发生在服务器。
 // 用动态多播是为了蓝图侧也能挂死亡表现（播动画 / 弹 UI）。
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOutOfHealthSignature);

UCLASS()
class ALPHA_API UAlphaAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UAlphaAttributeSet();

    // 属性复制注册：把各属性登记进网络复制系统
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // GE 生效后的统一回调：在此处对属性做钳制（0 ~ Max），并处理死亡等派生逻辑,仅在服务器执行。
    virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

	/** 生命值归零时广播（仅服务器）。监听方在此启动死亡流程。 */
	UPROPERTY(BlueprintAssignable, Category = "Attributes|Events")
	FOnOutOfHealthSignature OnOutOfHealth;

	// ========== 生命值 ==========
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Attributes|Vital")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UAlphaAttributeSet, Health);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Attributes|Vital")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UAlphaAttributeSet, MaxHealth);


	// ========== 法力值 ==========
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Mana, Category = "Attributes|Vital")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS_BASIC(UAlphaAttributeSet, Mana);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMana, Category = "Attributes|Vital")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS_BASIC(UAlphaAttributeSet, MaxMana);


	// ========== 耐力值 ==========
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "Attributes|Vital")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS_BASIC(UAlphaAttributeSet, Stamina);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina, Category = "Attributes|Vital")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS_BASIC(UAlphaAttributeSet, MaxStamina);



	// ========== OnRep 回调 ==========
	// 属性从服务器复制到本地后触发，负责刷新本地聚合器缓存并广播属性变化事件
	// 仅在非权威端（客户端 / 模拟代理）被引擎调用，服务器不会调用。

	/** Health 复制回调。@param OldHealth 复制前的旧值 */
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth) const;

	/** MaxHealth 复制回调。@param OldMaxHealth 复制前的旧值 */
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const;

	/** Mana 复制回调。@param OldMana 复制前的旧值 */
	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldMana) const;

	/** MaxMana 复制回调。@param OldMaxMana 复制前的旧值 */
	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana) const;

	/** Stamina 复制回调。@param OldStamina 复制前的旧值 */
	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldStamina) const;

	/** MaxStamina 复制回调。@param OldMaxStamina 复制前的旧值 */
	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina) const;


private:
	// 是否已越过生命归零临界点。多段伤害 / 周期扣血会反复进入
	// PostGameplayEffectExecute，靠这个标记保证死亡事件全局只广播一次。
	bool bOutOfHealth = false;
};
