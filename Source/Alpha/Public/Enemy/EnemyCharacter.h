#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Combat/AlphaAttributeSet.h"
#include "Components/WidgetComponent.h"
#include "EnemyCharacter.generated.h"

class UEnemyHealthBarWidget;
struct FOnAttributeChangeData;
class UWeaponComponent;
class UAlphaAttributeComponent;

// 敌人基类：实现 GAS 接口，挂 ASC + 属性集，作为攻击命中的受击目标。

UCLASS()
class ALPHA_API AEnemyCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

	// 重写 IAbilitySystemInterface：供武器命中后对目标应用伤害 GE
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// 死亡：血量为 0 时触发（变成布娃娃）
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void Die();

protected:
	virtual void BeginPlay() override;

	// 血量变化回调（事件驱动，归零触发死亡）
	void OnHealthChanged(const FOnAttributeChangeData& Data);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAlphaAttributeSet> AttributeSet;

	// 头顶血条组件（屏幕空间，始终面向摄像机）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HealthBar")
	TObjectPtr<UWidgetComponent> HealthBarComponent;

	// 血条 Widget 类（在敌人蓝图里指定 WBP_EnemyHealthBar）
	UPROPERTY(EditDefaultsOnly, Category = "HealthBar")
	TSubclassOf<UEnemyHealthBarWidget> HealthBarWidgetClass;

	// 武器组件（与玩家共用同一套逻辑）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponComponent> WeaponComponent;

	// 属性组件（GE 伤害应用入口，敌人攻击时用）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAlphaAttributeComponent> AttributeComponent;

private:

	bool bDead = false;   // 死亡标志
};