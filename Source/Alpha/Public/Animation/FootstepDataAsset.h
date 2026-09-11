#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FootstepDataAsset.generated.h"


/*
* 脚步声
* 数据结构 + 音效映射 DataAsset
*/

class USoundBase;
class UPhysicalMaterial;


// 单个脚步音效组：音效池 + 随机音量/音高范围
USTRUCT(BlueprintType)
struct FFootstepSoundGroup
{
	GENERATED_BODY()


	// 音效池，每次随机取一个播放
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	TArray<TObjectPtr<USoundBase>> Sounds;

	// 随机音量范围（X=最小，Y=最大）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	FVector2D VolumeRange = FVector2D(0.8f, 1.2f);

	// 随机音高范围（X=最小，Y=最大）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	FVector2D PitchRange = FVector2D(0.9f, 1.1f);

	// 触发概率（0~1）：每次脚步按此概率决定是否播放本组音效。
	// 1 = 每次必响；0.5 = 约一半脚步响。每档（走/跑/冲）各自独立。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PlayChance = 1.0f;

};


// 某个地面材质对应的三档（走/跑/冲）音效
USTRUCT(BlueprintType)
struct FFootstepSurfaceSounds
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	FFootstepSoundGroup WalkSounds;   // 行走音效

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	FFootstepSoundGroup RunSounds;    // 跑步音效

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	FFootstepSoundGroup SprintSounds;   // 冲刺音效
};


// 物理材质 → 音效 的映射条目
USTRUCT(BlueprintType)
struct FFootstepSurfaceMapping
{
	GENERATED_BODY()


	// 对应地面物理材质（如草地/石头/木板）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	TObjectPtr<UPhysicalMaterial> PhysicalMaterial;

	// 该材质下的三档音效
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	FFootstepSurfaceSounds Sounds;
};


// 脚步声音效配置资产（在编辑器里创建并填充）
UCLASS(BlueprintType)
class ALPHA_API UFootstepDataAsset : public UDataAsset
{
	GENERATED_BODY()


public:
	// 材质映射表（按物理材质查找对应音效）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	TArray<FFootstepSurfaceMapping> SurfaceMappings;

	// 盔甲叠加音效池（角色穿甲时，每次脚步额外叠加一层金属/布料声）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	FFootstepSurfaceSounds ArmorSounds;

	// 未匹配到任何材质时的兜底音效
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	FFootstepSurfaceSounds DefaultSounds;
};