#include "AlphaGameState.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Player/PlayerMaster.h"

void AAlphaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 复制给所有客户端：每个端都要弹结算界面，不能用 COND_OwnerOnly
	DOREPLIFETIME(AAlphaGameState, bGameOver);
}

void AAlphaGameState::SetGameOver()
{
	// 只有服务器能改：客户端本地改会被下一次属性复制覆盖，造成状态闪断
	if (!HasAuthority()) return;
	if (bGameOver) return;   // 幂等

	bGameOver = true;

	// 服务器上 OnRep 不会被引擎调用（只在接收端触发），
	// 所以这里直接广播，否则服务器本地（Listen Server 的 HUD）收不到通知。
	OnGameOverChanged.Broadcast();
}

void AAlphaGameState::ClearGameOver()
{
	// 只有服务器能改，理由同 SetGameOver：客户端本地改会被属性复制覆盖
	if (!HasAuthority()) return;
	if (!bGameOver) return;   // 幂等

	bGameOver = false;

	// 与 SetGameOver 对称：服务器上 OnRep 不会被调用，必须手动广播。
	// HUD 侧据此判断是「显示结算界面」还是「隐藏结算界面」——
	// 同一个委托承载双向变化，方向由 IsGameOver() 的当前值决定。
	OnGameOverChanged.Broadcast();
}

void AAlphaGameState::OnRep_GameOver()
{
	// 客户端：属性复制到达时广播
	OnGameOverChanged.Broadcast();
}

bool AAlphaGameState::AreAllPlayersDead() const
{
	// 没有玩家时不算团灭：开局 PlayerArray 尚未填充，会被误判为团灭
	if (PlayerArray.Num() == 0) return false;

	for (APlayerState* PS : PlayerArray)
	{
		if (!PS) continue;

		APawn* Pawn = PS->GetPawn();
		if (!Pawn) continue;   // 尚未生成 Pawn 或已掉线，不参与判定

		// 只要还有一个活着的玩家就不是团灭
		const APlayerMaster* Player = Cast<APlayerMaster>(Pawn);
		if (Player && !Player->IsDead())
		{
			return false;
		}
	}

	return true;
}