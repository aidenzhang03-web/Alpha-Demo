#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/FootstepDataAsset.h"
#include "FootstepNotify.generated.h"

/*
* 脚步声通知：在踩地帧触发，向下射线检测地面物理材质，播放对应音效。
* 走/跑/冲档位在运行时自动从角色读取，无需逐条动画手动指定。
*/

UCLASS(meta = (DisplayName = "脚步声通知"))
class ALPHA_API UFootstepNotify : public UAnimNotify
{
	GENERATED_BODY()

public:

	// 脚骨骼插槽名（foot_l / foot_r），用于定位脚的位置做向下检测
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	FName FootSocketName = "foot_l";

	// 脚步声音效配置资产
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	TObjectPtr<UFootstepDataAsset> FootstepData;

	// 向下检测距离（单位 cm，从脚位置向下扫）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep", meta = (ClampMin = "0"))
	float TraceDistance = 50.f;

	// 检测起点相对脚位置的向上偏移（避免脚刚好贴地时漏检，单位 cm）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep", meta = (ClampMin = "0"))
	float TraceStartOffset = 20.f;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

private:

	// 根据角色当前移动档位，从「走/跑/冲」三档音效里选出对应的一组
	const FFootstepSoundGroup* ResolveSoundGroup(const FFootstepSurfaceSounds& SurfaceSounds, AActor* Owner) const;
};