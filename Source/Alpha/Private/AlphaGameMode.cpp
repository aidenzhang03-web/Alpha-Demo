// Fill out your copyright notice in the Description page of Project Settings.


#include "AlphaGameMode.h"
#include "Player/PlayerMaster.h"
#include "AlphaGameState.h"


AAlphaGameMode::AAlphaGameMode()
{

	// 挂载自定义 GameState。不设这行，引擎会用默认的 AGameStateBase，
	// GetGameState<AAlphaGameState>() 永远返回 null，失败状态无处存放，
	// 而且不会有任何报错提示 —— 属于静默失效，很难排查。
	GameStateClass = AAlphaGameState::StaticClass();
}

// 玩家死亡通知：转交 GameState 判定是否团灭
void AAlphaGameMode::NotifyPlayerDied(APlayerMaster* DeadPlayer)
{
	// GameMode 本身只存在于服务器，这里是防御性写法
	if (!HasAuthority()) return;

	AAlphaGameState* GS = GetGameState<AAlphaGameState>();
	if (!GS) return;
	if (GS->IsGameOver()) return;   // 已判过失败，不重复结算

	// 每次有玩家死亡都重新判定一次。
	// 团灭的前提是「所有人都死了」，所以只有最后一个玩家死亡时才会成立，
	// 不能只看当前死亡的这一个。
	if (GS->AreAllPlayersDead())
	{
		GS->SetGameOver();
	}
}
