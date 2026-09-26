// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AlphaGameMode.generated.h"

class APlayerMaster;

/**
 * 
 */
UCLASS()
class ALPHA_API AAlphaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// 构造函数声明
	AAlphaGameMode();
	
	/**
	 * 玩家死亡通知入口。由 APlayerMaster::Die 在服务器调用。
	 * 只负责转交 GameState 做团灭判定，不含表现。
	 * @param DeadPlayer 刚死亡的玩家（当前未参与判定，留给后续记分/击杀归属用）
	 */
	void NotifyPlayerDied(APlayerMaster* DeadPlayer);
};
