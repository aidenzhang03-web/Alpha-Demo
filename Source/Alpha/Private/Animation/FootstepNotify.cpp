#include "Animation/FootstepNotify.h"

#include "Player/PlayerMaster.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundBase.h"
#include "Components/SkeletalMeshComponent.h"



void UFootstepNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !FootstepData) return;

	AActor* Owner = MeshComp->GetOwner();
	UWorld* World = MeshComp->GetWorld();

	if (!Owner || !World) return;

	// 1. 取脚骨骼位置
	const FVector FootLocation = MeshComp->GetSocketLocation(FootSocketName);

	// 2. 向下射线检测地面（从脚略上方开始，向下扫 TraceDistance）
	const FVector Start = FootLocation + FVector(0.f, 0.f, TraceStartOffset);
	const FVector End = FootLocation - FVector(0.f, 0.f, TraceDistance);

	FCollisionQueryParams Params;
	Params.bReturnPhysicalMaterial = true;  // 关键：命中时返回物理材质
	Params.AddIgnoredActor(Owner);          // 忽略角色自身，避免碰到胶囊体

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		return;  // 脚下无地面（腾空等），不发声

	// 3. 根据命中物理材质查映射表
	UPhysicalMaterial* PhysMat = Hit.PhysMaterial.Get();
	const FFootstepSoundGroup* SoundGroup = nullptr;

	if (PhysMat)
	{
		for (const FFootstepSurfaceMapping& Mapping : FootstepData->SurfaceMappings)
		{
			if (Mapping.PhysicalMaterial == PhysMat)
			{
				SoundGroup = ResolveSoundGroup(Mapping.Sounds, Owner);
				break;
			}
		}
	}

	// 未匹配到材质时，用默认音效兜底
	if (!SoundGroup)
		SoundGroup = ResolveSoundGroup(FootstepData->DefaultSounds, Owner);

	if (!SoundGroup || SoundGroup->Sounds.Num() == 0)
		return;

	// 4. 随机取音效 + 随机音量/音高
	const int32 SoundIndex = FMath::RandRange(0, SoundGroup->Sounds.Num() - 1);
	USoundBase* Sound = SoundGroup->Sounds[SoundIndex];
	const float Volume = FMath::RandRange(SoundGroup->VolumeRange.X, SoundGroup->VolumeRange.Y);
	const float Pitch = FMath::RandRange(SoundGroup->PitchRange.X, SoundGroup->PitchRange.Y);

	// 5. 在命中点播放
	UGameplayStatics::PlaySoundAtLocation(World, Sound, Hit.Location, Volume, Pitch);

	// 6. 盔甲叠加层：按当前档位选走/跑/冲对应的盔甲声，与地面材质解耦（任何地面都触发）
	const FFootstepSoundGroup* ArmorGroup = ResolveSoundGroup(FootstepData->ArmorSounds, Owner);
	if (ArmorGroup && ArmorGroup->Sounds.Num() > 0)
	{
		// 每个档位（走/跑/冲）各自独立的触发概率
		if (FMath::FRand() <= ArmorGroup->PlayChance)
		{
			const int32 ArmorIdx = FMath::RandRange(0, ArmorGroup->Sounds.Num() - 1);
			const float ArmorVol = FMath::RandRange(ArmorGroup->VolumeRange.X, ArmorGroup->VolumeRange.Y);
			const float ArmorPitch = FMath::RandRange(ArmorGroup->PitchRange.X, ArmorGroup->PitchRange.Y);
			UGameplayStatics::PlaySoundAtLocation(
				World, ArmorGroup->Sounds[ArmorIdx], Hit.Location, ArmorVol, ArmorPitch);
		}
	}

}


const FFootstepSoundGroup* UFootstepNotify::ResolveSoundGroup(const FFootstepSurfaceSounds& SurfaceSounds, AActor* Owner) const
{
	// 默认按跑步处理；拿到角色实例后读权威档位
	EMoveSpeedState State = EMoveSpeedState::Run;
	if (APlayerMaster* Player = Cast<APlayerMaster>(Owner))
		State = Player->GetMoveSpeedState();


	switch (State)
	{
	case EMoveSpeedState::Walk:
		return &SurfaceSounds.WalkSounds;

	case EMoveSpeedState::Sprint:
		return &SurfaceSounds.SprintSounds;

	case EMoveSpeedState::Run:
	default:
		return &SurfaceSounds.RunSounds;
	}
}