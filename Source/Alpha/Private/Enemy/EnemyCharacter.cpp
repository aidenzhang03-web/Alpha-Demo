#include "Enemy/EnemyCharacter.h"
#include "UI/EnemyHealthBarWidget.h"
#include "AI/EnemyAIController.h"
#include "Weapon/WeaponComponent.h"
#include "Combat/AlphaAttributeComponent.h"
#include "Combat/AlphaGameplayTags.h"
#include "Combat/GA_EnemyAttack.h"
#include "Combat/GA_HitReact.h"
#include "Animation/AlphaAnimNotify.h"
#include "Animation/AlphaAnimNotifyState.h"

#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffectTypes.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UAlphaAttributeSet>(TEXT("AttributeSet"));

	// 头顶血条组件
	HealthBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarComponent"));
	HealthBarComponent->SetupAttachment(RootComponent);

	// 屏幕空间：血条始终面向摄像机（头顶血条标准做法）
	HealthBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	// 血条在屏幕上的绘制尺寸（像素），按需在蓝图里微调
	HealthBarComponent->SetDrawSize(FVector2D(150.f, 18.f));
	// 放到头顶（相对胶囊体向上偏移，具体值按角色身高微调）
	HealthBarComponent->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
	// 血条不参与碰撞、不接收输入
	HealthBarComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// AI 控制：由自定义 AIController 接管；放置到关卡/生成时自动附身
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 武器组件 + 属性组件
	WeaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComponent"));
	AttributeComponent = CreateDefaultSubobject<UAlphaAttributeComponent>(TEXT("AttributeComponent"));
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 初始化 ASC 的 ActorInfo（Owner 与 Avatar 均为自身）。
	// 不调用这行，AttributeSet 不会被 ASC 注册，伤害 GE 会静默失效。
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		if (AttackAbilityClass)
			AbilitySystemComponent->GiveAbility(
				FGameplayAbilitySpec(AttackAbilityClass, 1, INDEX_NONE, this));

		if (HitReactAbilityClass)
			AbilitySystemComponent->GiveAbility(
				FGameplayAbilitySpec(HitReactAbilityClass, 1, INDEX_NONE, this));

		// 监听血量变化：归零触发死亡（必须在 InitAbilityActorInfo 之后，AttributeSet 才已注册）
		if (const UAlphaAttributeSet* AttrSet = AbilitySystemComponent->GetSet<UAlphaAttributeSet>())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttrSet->GetHealthAttribute())
				.AddUObject(this, &AEnemyCharacter::OnHealthChanged);
		}

	}


	// 初始化头顶血条：确保 Widget 已按指定类创建
	if (HealthBarComponent && HealthBarWidgetClass)
	{
		// 若蓝图里已给组件配了 Widget Class，则无需重建；否则手动设置并初始化
		if (!HealthBarComponent->GetUserWidgetObject())
		{
			HealthBarComponent->SetWidgetClass(HealthBarWidgetClass);
			HealthBarComponent->InitWidget();
		}
	}


	// 开局生成武器并挂到背部插槽
	if (WeaponComponent)
		WeaponComponent->SpawnAndAttachWeapon();


	// 拿到血条 Widget 实例，绑定 ASC 监听血量变化
	if (UEnemyHealthBarWidget* Bar = Cast<UEnemyHealthBarWidget>(
		HealthBarComponent ? HealthBarComponent->GetUserWidgetObject() : nullptr))
	{
		Bar->InitializeHealthBar(AbilitySystemComponent);
	}
}

UAbilitySystemComponent* AEnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}


// 血量变化回调：归零触发死亡
void AEnemyCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (bDead) return;   // 已死亡，忽略死亡后的残余事件

	// 掉血且未致死 → 发受击事件，由 UGA_HitReact 监听并播放受击动画
	if (Data.NewValue < Data.OldValue && Data.NewValue > 0.f)
	{
		FGameplayEventData Payload;
		Payload.EventTag = AlphaGameplayTags::Event_HitReact;
		Payload.Target = this;
		Payload.Instigator = this;
		Payload.EventMagnitude = Data.OldValue - Data.NewValue;   // 本次伤害量（预留：可用于分段受击）
		AbilitySystemComponent->HandleGameplayEvent(AlphaGameplayTags::Event_HitReact, &Payload);
	}

	// 归零 → 死亡（放在受击判定之后，且用 > 0 卡掉致死那一下的受击）
	if (Data.NewValue <= 0.f)
		Die();
}


// 死亡：停止移动、关闭胶囊体、开启骨骼物理（布娃娃）
void AEnemyCharacter::Die()
{
	if (bDead) return;
	bDead = true;

	// 1. 停止并禁用移动（否则角色会继续站立/移动）
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
		Move->SetComponentTickEnabled(false);
	}

	// 2. 关闭胶囊体碰撞（否则胶囊体还撑着角色、挡子弹/攻击）
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 3. 开启骨骼网格体物理模拟（布娃娃核心）
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));  // 改用 Ragdoll 碰撞预设，让骨骼体与地面 Block
		MeshComp->SetSimulatePhysics(true);              // 开启物理模拟
		MeshComp->SetAllBodiesSimulatePhysics(true);     // 所有骨骼体都模拟
		MeshComp->WakeAllRigidBodies();                  // 唤醒刚体，防止“睡着”
	}

	// 4. 隐藏头顶血条（死亡后不再显示）
	if (HealthBarComponent)
	{
		HealthBarComponent->SetVisibility(false);
	}

	// 5. 关闭自身 Tick（死亡后无需再更新）
	SetActorTickEnabled(false);

	// 倒地后几秒销毁尸体（按需打开）
	 SetLifeSpan(3.f);
}


// 单帧动画事件
void AEnemyCharacter::HandleAnimEvent(EAnimEventType EventType)
{
	switch (EventType)
	{
		// ======== 拔出/收起武器（与玩家同一套逻辑）========
	case EAnimEventType::WeaponAttachHand:
		if (WeaponComponent) WeaponComponent->AttachWeaponToHand();
		break;

	case EAnimEventType::WeaponAttachBack:
		if (WeaponComponent) WeaponComponent->AttachWeaponToBack();
		break;
	default:
		break;
	}
}


// 区间动画状态：进入
void AEnemyCharacter::HandleAnimStateBegin(EAnimNotifyStateType StateType)
{
	switch (StateType)
	{
	case EAnimNotifyStateType::AttackHitWindow:
		// 攻击命中窗口打开：开启武器碰撞盒
		if (WeaponComponent) WeaponComponent->EnableWeaponHitbox();
		break;
	default:
		break;
	}
}


// 区间动画状态：退出
void AEnemyCharacter::HandleAnimStateEnd(EAnimNotifyStateType StateType)
{
	switch (StateType)
	{
	case EAnimNotifyStateType::AttackHitWindow:
		// 攻击命中窗口关闭：关闭武器碰撞盒
		if (WeaponComponent) WeaponComponent->DisableWeaponHitbox();
		break;
	default:
		break;
	}
}