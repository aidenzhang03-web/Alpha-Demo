#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyHealthBarWidget.generated.h"

class UProgressBar;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;

// 敌人头顶血条基类：监听 GAS 血量变化刷新血条，视觉布局在 Widget 蓝图子类完成。

UCLASS()
class ALPHA_API UEnemyHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// 绑定敌人 ASC 并注册血量监听（由敌人 Actor 在 BeginPlay 调用一次）
	UFUNCTION(BlueprintCallable, Category = "HealthBar")
	void InitializeHealthBar(UAbilitySystemComponent* InASC);


protected:

	// 血条进度条（Widget 蓝图中同名控件自动绑定，名字必须叫 HealthBar）
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	// 血量变化回调（事件驱动，不在 Tick 轮询）
	void OnHealthChanged(const FOnAttributeChangeData& Data);

	// 初次刷新
	void RefreshBar();


private:

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	bool bInitialized = false;

};