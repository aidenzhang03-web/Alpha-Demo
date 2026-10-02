# Alpha 开发踩坑记录

> 本文档记录 Alpha 开发过程中**实际定位过**的问题，按「现象 → 根因 → 修法 → 教训」四段组织。
> 每条都尽量给出引擎源码依据或代码位置（行号对应当时的提交），便于回查。
> 按模块分类，越靠前的排错价值越高。

## 目录

| 分类 | 条目 |
|---|---|
| 一、GAS 连招与能力时序 | 7 |
| 二、网络复制 | 6 |
| 三、动画 / Motion Matching | 3 |
| 四、引擎 API 与静默失败 | 5 |

---

## 一、GAS 连招与能力时序

### 1. 连招幽灵输入：第一段播完自动接第二段

**现象**：只按一次左键，L1 播放结束后自动进入 L2。

**根因**：`UAbilitySystemComponent::HandleGameplayEvent`（`AbilitySystemComponent_Abilities.cpp`，约 2569-2602 行）的执行顺序是**先触发能力、后广播事件**：

```text
第 2569-2585 行：TriggerAbilityFromGameplayEvent → ActivateAbility
                 （能力内部创建 WaitGameplayEvent 并 ReadyForActivation）
第 2587-2602 行：才向监听器广播该事件
```

于是「触发激活的那一次按键」在能力激活后，又被内部刚创建的 `WaitGameplayEvent` 重复捕获一次，调用 `OnAttackInput` 把 `bHasBufferedInput` 置为 `true`，L1 结束就自动接上 L2。

**修法**：时间戳去抖。

- `GA_Attack.h` 增加成员 `float ActivationWorldTime = 0.f;`
- `ActivateAbility` 重置连招状态处记录 `ActivationWorldTime = GetWorld()->GetTimeSeconds();`
- `OnAttackInput` 开头拦截：`if (GetWorld() && GetWorld()->GetTimeSeconds() <= ActivationWorldTime) return;`

**教训**：GAS 中不要用同一个 GameplayEvent 既做激活触发又做连段触发，否则会因「先激活后广播」的分发顺序产生同帧幽灵输入。真实人类的连击不可能与激活同帧，用时间戳去抖即可规避，不必改事件设计。

<br>

### 2. 事件泄漏：新段任务误收旧段的延迟事件

**现象**：`bInComboWindow` 卡在 `true`，或新段刚开就被判定「窗口结束」。

**根因**：`AnimNotifyState::NotifyEnd` 是**帧级延迟**触发（非同步）。`AdvanceCombo` 中先 `EndTask` 旧段 `WaitWindowEnd`，再停旧段 Montage、`PlaySection` 新段；但旧段接续窗口的 `NotifyEnd` 要到下一帧才发出 `Combo.WindowEnd`，此时新段的 `WaitWindowEnd`（`OnlyTriggerOnce = true`，`GA_Attack.cpp:103`）已创建并误收该延迟事件后自动结束，导致新段真正的 `WindowEnd` 无人接收。

**修法**：新段任务需过滤掉延迟到达的旧段事件。

验证手法：在 `OnComboWindowEnd` 打印 `bInComboWindow`，若收到 `WindowEnd` 时该值为 0 即实锤泄漏。

**教训**：UE 的 Notify / NotifyEnd 不是同步回调，不能假设「停掉任务就不会再收到它的事件」。

<br>

### 3. EndTask 不能停止 Montage

**现象**：连招中断后动画仍继续播完；或提前 `EndTask` 后能力结束，动画却停不下来。

**根因**：`UAbilityTask_PlayMontageAndWait::EndTask()` 走 `OnDestroy(false)`，**不会停止 Montage**；只有 `EndAbility()` 触发的 `OnDestroy(true)` 才会停。

**修法**：`EndCombo()` 里不提前 `EndTask`，只解绑回调 + 清引用，让 `EndAbility()` 负责停 Montage。

**教训**：能力任务 `EndTask` 的语义是「结束监听」而非「回滚副作用」，副作用收尾要交给 `EndAbility`。项目已在 `GA_HitReact.cpp:96` 留下约定注释（见 `montage-stop-convention`）。

<br>

### 4. 复活后角色仍保持倒地姿态

**现象**：复活后姿态没有恢复，仍停在死亡动画的最后一帧。

**根因**：死亡蒙太奇的 Slot 权重没有被复位（权重一直为 1），姿态被死亡动画持续覆盖。蒙太奇播完不等于权重归零。

**修法**：复活流程中显式 `Montage_Stop`（相关说明见 `PlayerMaster.cpp:608-610` 附近注释）。

**教训**：涉及姿态残留时，不能依赖「动画播完自动回落」，要显式停。

<br>

### 5. LooseGameplayTag 不参与复制

**现象**：本地玩家的攻击输入仍能激活连招（客户端预测），随后被服务器拒绝，产生动画回滚抖动。

**根因**：`SetLooseGameplayTagCount` 授予的标签**不参与网络复制**，只有服务器端持有 `State.Dead`；客户端看不到死亡状态，`LocalPredicted` 的连招照常激活。

**修法**：在 `Multi_Die`（NetMulticast）中让各端各自授予 `State.Dead`，使客户端本地也能拦住激活请求。

**教训**：本地预测能力的门禁依赖「本地是否存在某标签」，凡是不复制的标签，都要靠多播在各端补齐。

<br>

### 6. 标签计数语义：SetLooseGameplayTagCount 与 Update 的区别

**现象**：重复调用后标签计数异常，死亡 / 复活状态判断失准。

**根因**：`SetLooseGameplayTagCount` 是**幂等设定**（直接设为指定值），而递减式调用每次都会改变计数。两者混用或重复调用会破坏计数语义。

**修法**：状态位统一用幂等方式 `SetLooseGameplayTagCount(..., 1)`（`PlayerMaster.cpp:583-585` 有注释说明）。

**教训**：GAS 中标签计数有「设定」与「累加/递减」两种语义，不要当成同一个 API 使用。

<br>

### 7. CanActivateAbility 签名与死亡拦截的标签查询

**现象**：覆写编译报 C3668（未重写基类虚函数）+ C2664（实参无法转换）。

**根因**：`UGameplayAbility::CanActivateAbility` 是 **5 参数**签名：

```cpp
bool CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const;
```

容易与 `DoesAbilitySatisfyTagRequirements` 的签名（两个 TagContainer 引用）混淆。

**修法**：按 5 参数签名覆写；判断死亡状态应查 `ASC->HasMatchingGameplayTag(State_Dead)`，而**不要**读 `SourceTags` / `TargetTags`（按键激活时它们恒为 `nullptr`）。

**教训**：覆写引擎虚函数前先读头文件确认签名，不要凭记忆。

---

## 二、网络复制

### 8. RepNotify 静默失效：客户端收不到死亡通知

**现象**：客户端 HUD 收不到死亡通知，个人死亡面板不显示。

**根因**：`bDead` 同时承担「复制属性」与「本地表现状态」两个角色。`Multi_Die` 已在各端本地赋值 `bDead = true`，属性复制到达时新旧值相同，`OnRep_` 不再触发。

**修法**：改用 `OnDeadStateChanged` 多播委托，在 `Multi_Die` / `Multi_Revive` 内主动广播（取舍记录见 `PlayerMaster.h:206-220`）。

**教训**：当「属性复制」与「多播本地赋值」覆盖同一状态时，RepNotify 会被吃掉，需要一个显式的广播通道。

<br>

### 9. 服务器上 GetLastInputVector 恒为零向量

**现象**：服务器侧判断「远程客户端是否在移动」永远为否。

**根因**：调用链如下 ——

```text
PawnMovementComponent.cpp:77
  → APawn::GetLastMovementInputVector()          (Pawn.cpp:841)
     返回 LastControlInputVector
唯一写入点：APawn::Internal_ConsumeMovementInputVector()  (Pawn.cpp:869)
     无条件执行 LastControlInputVector = ControlInputVector;
而 ControlInputVector 只在 APawn::AddMovementInput 路径累加。
客户端输入是经 ServerMove 的 NewAccel 参数上行的，不经过这条路径。
```

**修法**：使用 `MoveAutonomous`（`CharacterMovementComponent.cpp:10664`）恢复出的 `GetCurrentAcceleration()`。

**教训**：服务器侧拿不到客户端的「输入向量」，只能拿到移动组件恢复出的加速度 / 速度。

<br>

### 10. 属性被钳制后 UI 不同步

**现象**：属性被钳制回原值（例如超量伤害打在满血目标上）时，客户端 UI 不刷新。

**根因**：默认的 `REPNOTIFY_OnChanged` 只在值发生变化时触发；「钳制回原值」时新旧值相同，`OnRep` 不触发，但 UI 确实需要同步。

**修法**：在 `GetLifetimeReplicatedProps` 中使用 `DOREPLIFETIME_CONDITION_NOTIFY(..., COND_None, REPNOTIFY_Always)`（`AlphaAttributeSet.cpp:60-72`）。

**教训**：涉及钳制 / 派生逻辑的属性，复制通知要用 `Always`。

<br>

### 11. COND_OwnerOnly 误用

**现象**：远端角色动画不对、血条不更新。

**根因**：把「远端也需要读」的状态按 `OwnerOnly` 复制，非 Owner 客户端拿不到数据。

**修法**：需要跨端读取的状态使用 `COND_None`（见 `AlphaGameState.cpp:11`、`PlayerMaster.cpp:199/205` 的注释：「不能用 COND_OwnerOnly」）。

**教训**：复制条件的判断依据是「谁需要读」，不是「谁产生的」。

<br>

### 12. 复活不清理 GameOver 会让团灭判定永久失效

**现象**：第一次团灭结算后，之后再有人死亡都不会再触发结算。

**根因**：`bGameOver` 停留在 `true`，而 `NotifyPlayerDied` 有早退逻辑，后续判定被整体跳过。

**修法**：`Revive()` 中调用 `AAlphaGameState::ClearGameOver()`（`PlayerMaster.cpp:568` 附近）。

**教训**：全局状态位必须有明确的复位路径，否则早退逻辑会把它变成死锁。

<br>

### 13. 权威轨迹慢于客户端预测

**现象**：客户端位置被 `ClientAdjustPosition` 拉回，出现位置回抽。

**根因**：权威轨迹比客户端预测慢，误差累积后触发移动校正，把客户端拉回慢速轨迹（`PlayerMaster.cpp:309` 有说明）。

**修法**：本项目采用**服务器权威 + 不做客户端预测**的路线，靠复制对齐状态，从设计上避免预测回滚。

**教训**：不做预测就要接受输入延迟，做预测就必须处理校正 —— 两者不可兼得，需在项目初期定好路线。

---

## 三、动画 / Motion Matching

### 14. 远端 Acceleration 恒为 0 → 停止过渡小碎步

**现象**：**仅远端**角色（在另一客户端视口观察）停止时，脚在原地快速倒腾；本地角色正常。

**根因**：`UCharacterMovementComponent::Acceleration` 只在 `CalcVelocity`（`PerformMovement` 内，引擎源码约 3890 / 3894 / 3993-4005 行）被赋值；SimulatedProxy 走 `SimulatedTick` → `SmoothCorrection`，**不执行 PerformMovement**。且引擎**没有** `ReplicatedAcceleration` 成员或复制开关（grep 引擎源码 0 命中）。

→ **远端代理的 Acceleration 恒为 0**。

**后果**：Motion Matching 的 Trajectory 预测公式

$$v(t) = v_0 + a \cdot t$$

其中 a = 0 时，远端预测「等速直冲」、本地预测「减速收敛」→ 两端选帧不同；停止动画各帧的未来位置递减，远端查询「增长快」→ 每帧都匹配到动画起始段 → 脚在原地快速倒腾。

**修法**：

- `APlayerMaster` 增加 `UPROPERTY(Replicated) FVector AccelerationAuth;`
- 服务器 Tick 的 `HasAuthority()` 分支内：`AccelerationAuth = GetCharacterMovement()->GetCurrentAcceleration();`
- `UPlayerAnimInstance` 远端分支末尾：`PlayerMovementComponent->Acceleration = Player->GetAccelerationAuth();`

（`Acceleration` 是 public UPROPERTY，SimulatedProxy 的物理不消费它，不会干扰位置平滑。）

**教训**：引擎里「本地模拟」与「远端插值」走的是不同代码路径。凡是只在 `PerformMovement` 中更新的数据，远端一律没有，需要显式复制。

<br>

### 15. 伪造移动方向导致腿部扭曲（碎步）

**现象**：远端角色减速 / 转向时腿部被按错误角度扭曲，视觉上表现为碎步。

**根因**：`FullVelocity` 曾用 `ActorForwardVector * Speed` 计算，即**伪造方向**。而 Orientation Warping 的公式是：

$$Orientation = RotationBetween(RootMotionDirection,\ LocomotionDirection)$$

方向不真实时与真实移动方向存在夹角；且速度归零时该向量退化为零向量。

**修法**：`PlayerAnimInstance.cpp:117-143` 改为「**幅值取权威 `SpeedAuth` + 方向取真实复制速度**」：

```cpp
const FVector MoveDir = ReplicatedVelocity.SizeSquared2D() > KINDA_SMALL_NUMBER
    ? ReplicatedVelocity.GetSafeNormal2D()
    : Player->GetActorForwardVector();     // 速度极小时方向无意义，回退到朝向

MovementSpeed = Player->GetSpeedAuth();
FullVelocity = MoveDir * MovementSpeed;
```

**教训**：动画 Warping 的输入必须是**真实运动方向**，不能用朝向凑；否则问题只在减速 / 转向这类「朝向与速度不一致」的时刻显现，很难排查。

<br>

### 16. PoseSearch 调试 CVar 中有「空壳」

**现象**：按文档设置了 `a.AnimNode.MotionMatching.DebugDrawQuery`，却没有任何绘制。

**根因**：该 CVar 与 `DebugDrawCurResult` 只做了声明 + 注册，全文件**没有任何读取点**（`AnimNode_MotionMatching.cpp:39-43`），属于空壳。

**修法**：使用唯一有效的 `a.AnimNode.MotionMatching.DebugDrawInfo`（使用点 `:121`），配合 `DebugDrawInfoVerbose`（默认 true）、`DebugDrawInfoHeight`（默认 50），可在角色头顶绘制当前数据库 / 动画资产名。

其他可用工具：`a.AnimNode.PoseHistory.DebugDrawPose`；Warping 节点自带 `bEnableDebugDraw`。

**教训**：调引擎调试 CVar 前先确认它有读取点，否则会浪费大量排查时间。

---

## 四、引擎 API 与静默失败

### 17. GameState 未到达导致绑定被静默跳过

**现象**：HUD 已创建，但血条 / 结算面板都不更新，且**无任何报错**。

**根因**：HUD 在 `OnRep_Controller` 中创建时，`GameState` 可能尚未复制到达，`GetGameState()` 返回 `nullptr`，导致整个绑定逻辑块被跳过（`if (GS) { ... }` 无声跳过）。

**修法**：使用 `TryBindGameState` + 0.1s Timer 重试，确保 GameState 到达后再绑定。

**教训**：排查「无报错但功能不生效」时，从链路**终点往前推**，在每一环找「有没有代码会静默什么都不做」的地方；配合关键节点日志（如 `[GS]` / `[HUD]`）做二分，可快速定位断点。

<br>

### 18. BlueprintImplementableEvent 未实现 → 无声跳过

**现象**：结算面板不弹出，无报错。

**根因**：`BlueprintImplementableEvent` 在蓝图中未实现时，调用不会有任何反应，也无警告。

**修法**：确认蓝图侧已实现该事件；对关键流程在 C++ 侧提供默认兜底行为。

**教训**：C++ → 蓝图的单向接口没有编译期保护，属于典型的静默失败点。

<br>

### 19. 构造函数里的参数被蓝图覆盖

**现象**：`BrakingDecelerationWalking` 变成 0，刹车失效。

**根因**：写在 C++ 构造函数中的 `CharacterMovementComponent` 参数，会被蓝图资产中**序列化保存的值**覆盖。

**修法**：把这类「必须生效」的参数从构造函数挪到 `BeginPlay` 中强制设置。

**教训**：C++ 构造函数不是配置的最终裁决者 —— 蓝图资产的反序列化发生在更晚的阶段，优先级更高。

<br>

### 20. UE 5.8 GameplayEffect 字段改名

**现象**：按旧教程找 `Add` 选项找不到。

**根因**：UE 5.8 的 `EGameplayModOp`（`GameplayEffectTypes.h:112-149`）已无单独的 `Add`，改为：

| 5.8 中的名称 | 说明 |
|---|---|
| `Add (Base)` | 旧版 `Add`，通用扣属性 / 扣耐用它 |
| `Multiply (Additive)` | |
| `Divide (Additive)` | |
| `Multiply (Compound)` | 新增 |
| `Add (Final)` | 新增 |
| `Override` | |

补充：GameplayAbilities 插件没有 Localization 目录，GE 字段的 UPROPERTY 没有 DisplayName，界面**永远显示英文**（驼峰拆词），不要去找中文翻译。

**修法**：扣血 / 扣耐统一使用 `Add (Base)`。

**教训**：引擎小版本会改枚举显示名，升级后以**实际界面**为准，不要沿用旧教程的选项名。

<br>

### 21. ESlateVisibility 枚举名与编辑器显示名相反

**现象**：想让按钮可点击，结果控件点不动。

**根因**：C++ 枚举名与编辑器下拉显示名的对应关系是**反的**：

| C++ 枚举名 | 编辑器下拉显示名 |
|---|---|
| `HitTestInvisible` | `Not Hit-Testable (Self Only)` |
| `SelfHitTestInvisible` | `Not Hit-Testable (Self & All Children)` |

其中 `SelfHitTestInvisible` 会**连同子级一起屏蔽命中测试**，所以用它「保留子级可点击」会失效。

**修法**：需要保留子级可点击时用 `HitTestInvisible`；仅隐藏自身显示、保留命中用 `Hidden`。

**教训**：不要凭枚举名猜语义，以下拉框实际文本为准。

---

## 附：可复用的排错手法

1. **从链路终点往前推**：在每一环找「有没有代码会静默什么都不做」的地方（`if (指针)`、未实现的蓝图事件、未到达的复制数据）。
2. **日志二分**：在链路关键节点打日志，把链路切成确定区间，快速定位断点。
3. **区分「本地」与「远端」**：引擎中两者走不同代码路径（`PerformMovement` vs `SimulatedTick`），现象只在一侧出现时，优先怀疑路径差异。
4. **先确认 API 有读取点 / 有复制通道**：CVar、`Replicated` 属性都存在「看起来能用、实际是空壳」的情况。
5. **以源码为准**：枚举名、函数签名、字段名都随版本变化，出问题先读本地引擎源码。
