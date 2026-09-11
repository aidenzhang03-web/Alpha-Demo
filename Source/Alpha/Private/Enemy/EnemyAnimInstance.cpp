#include "Enemy/EnemyAnimInstance.h"
#include "GameFramework/Pawn.h"

void UEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{


	Super::NativeUpdateAnimation(DeltaSeconds);

	APawn* Pawn = TryGetPawnOwner();
	if (!Pawn)
	{
		Speed = 0.f;
		bIsMoving = false;
		Direction = 0.f;
		return;
	}

	// 取水平速度（忽略 Z，走地面时 Z 基本为 0）
	const FVector Velocity = Pawn->GetVelocity();
	Speed = Velocity.Size2D();
	bIsMoving = Speed > 3.f;

	// 计算移动方向（相对角色朝向），供后续转向/侧移动画使用
	if (bIsMoving)
	{
		const FVector LocalVel = Pawn->GetActorTransform().InverseTransformVectorNoScale(Velocity);
		Direction = FMath::RadiansToDegrees(FMath::Atan2(LocalVel.Y, LocalVel.X));
	}
	else
	{
		Direction = 0.f;
	}
}