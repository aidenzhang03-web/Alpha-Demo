// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h" 
#include "Components/BoxComponent.h"
#include "WeaponBase.generated.h"

// 武器类型枚举
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	Spear       UMETA(DisplayName = "长枪"),
	Sword       UMETA(DisplayName = "单手剑"),
	Greatsword  UMETA(DisplayName = "双手剑")
};

//武器命中委托，参数为命中的目标 Actor
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponHit, AActor*, HitActor);

UCLASS()
class ALPHA_API AWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponBase();

	// 武器网格组件（骨骼网格体，在武器蓝图中指定具体 SkeletalMesh）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	// 静态网格体武器（静态网格体，在武器蓝图中指定具体 StaticMesh）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponStaticMesh;

	// 武器类型（供逻辑 / UI / 动画层判断）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	EWeaponType WeaponType = EWeaponType::Spear;

	//攻击判定盒（默认关闭碰撞，命中窗口内才开启）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UBoxComponent> HitboxComponent;

	//命中事件（供 WeaponComponent 绑定）
	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnWeaponHit OnWeaponHit;

	//开启 / 关闭攻击判定盒
	void EnableHitbox();
	void DisableHitbox();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//碰撞盒 Overlap 回调
		UFUNCTION()
	void OnHitboxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
