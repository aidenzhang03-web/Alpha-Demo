# Alpha

> 基于 **Unreal Engine 5.8 + C++** 开发的第三人称动作游戏 Demo。
> 移动动画由 **Motion Matching（Pose Search）** 驱动，战斗与属性系统基于 **GAS（Gameplay Ability System）** 实现。
>
> **本仓库仅用于展示 C++ 实现**：只包含 `Source/` 源码与 `Config/` 工程配置，
> 美术 / 蓝图 / 动画等 `Content/` 资产不入库（详见「快速开始」）。

---

## 技术栈

| 类别 | 使用 |
|---|---|
| 引擎 | Unreal Engine 5.8 |
| 语言 | C++（配合 Enhanced Input / UMG 蓝图） |
| 动画 | Motion Matching（Pose Search）+ Chooser |
| 能力系统 | Gameplay Ability System（GAS） |
| AI | Behavior Tree · Blackboard · Navigation System |
| 网络 | Actor / 属性复制 + 服务器权威判定（GAS） |
| 版本管理 | Git + Git LFS |

---

## 核心特性

### 移动
- 三档位（走 / 跑步 / 冲刺）点按切换
- 事件驱动的速度与加速度管理，加速度参数按档位拆分
- 惯性滑行刹车（恒定减速度，松键后平滑减速）
- 折返转向判定与平滑转身

### 动画（Motion Matching）
- **Chooser 选库 + Pose Search 逐帧搜索**：Chooser 表按运动状态选择动画数据库（PSD），Motion Matching 在库内搜索最优匹配帧
- 统一 **Pose Search Schema** + **Mirror Data Table**（支持左右镜像动画）
- 依赖 **continuing pose（延续姿态）** 实现档位间（走 ↔ 跑 ↔ 冲刺）平滑过渡，而非硬编码状态机过渡
- 数据驱动脚步音效：按地面物理材质 + 走/跑/冲档位自动匹配音效，支持盔甲叠加层

### 战斗（GAS 连招）
- 左右键各一套连招，每套 6 段（基础 4 段 + 衍生 2 段），共用全局段号计数器
- 三层输入窗口设计：
  - **缓冲窗口**：攻击段内提前按键会被缓冲
  - **接续窗口**：到达可打断点后立即接下一段（跳过收招，保证手感）
  - **衍生窗口**：第 2 段收招停顿后按键 → 进入衍生分支
- **事件驱动中断**：移动 / 跳跃发送 `Combo.InterruptRequest` 事件，由能力自身按「缓冲优先于中断」决策是否结束

### 战斗闭环
```
连招能力 → 攻击判定窗口(AnimNotifyState) → 开启武器碰撞盒
→ Overlap 命中敌人 → 应用伤害 GameplayEffect → 敌人扣血 / 死亡
```

### 属性
- 生命 / 法力 / 耐力属性集
- 统一通过 GameplayEffect 修改（冲刺持续耗耐、延迟自动恢复）

### 敌人
- 实现 GAS 接口，挂载 ASC + 属性集
- 头顶血条（事件驱动，非 Tick 轮询）
- 视觉感知（AIPerception）→ 黑板写目标 → 行为树追击 / 攻击（攻击能力 `GA_EnemyAttack`）
- 受击反馈：打断当前攻击、受击期间锁移动防滑步、致死直接布娃娃（`GA_HitReact`）
- 死亡布娃娃 + 尸体自动销毁（`Multi_Die` 多播广播表现）

### 武器
- 组件化多武器切换（配置驱动，新增武器无需改动组件代码）
- 收拔刀动画 + 骨骼过滤混合（站立全身播放 / 移动仅上半身）

### 网络（Replication）
- Actor 与移动复制；ASC 开启复制（`Mixed` 模式：属性同步给所有客户端，GE 明细只给 Owner）
- 属性集通过 `GetLifetimeReplicatedProps` + `OnRep_*` 同步，血条跨端一致
- **服务器权威**：命中判定仅在服务器开关 Hitbox；属性应用设有 `HasAuthority` 根防线
- 能力网络策略：连招 `LocalPredicted`、敌人攻击 `ServerOnly`
- 敌人死亡拆分为权威判定 + `Multi_Die()`（NetMulticast Reliable）表现广播
- 远端角色动画按 `IsLocallyControlled()` 分流，修正 SimulatedProxy 误播 Idle

---

## 项目结构

```
Alpha/
├── Config/                  # 引擎与输入配置
├── Content/
│   └── Alpha/               # 本项目资产（角色 / 动画 / 武器 / 蓝图 / UI）
├── Plugins/
├── Source/
│   ├── Alpha.Target.cs
│   ├── AlphaEditor.Target.cs
│   └── Alpha/
│       ├── Alpha.Build.cs
│       ├── Public/
│       │   ├── AI/          # 敌人 AI 控制器
│       │   ├── Animation/   # 动画通知 / 脚步音效
│       │   ├── Combat/      # GAS：属性 / 能力 / 标签 / 连招
│       │   ├── Enemy/       # 敌人角色与动画实例
│       │   ├── Player/      # 玩家角色 / 输入组件 / 动画实例
│       │   ├── UI/          # HUD 与敌人血条
│       │   ├── Weapon/      # 武器基类与武器组件
│       │   └── AlphaGameMode.h
│       └── Private/         # 实现文件（与 Public 结构镜像）
└── Alpha.uproject
```

---

## 环境要求（阅读 / 编译源码所需）

- **Unreal Engine 5.8**（用于编译源码与打开编辑器）
- **Visual Studio 2022**（需勾选「使用 C++ 的游戏开发」工作负载）
- Windows 10 / 11
- Git + Git LFS（仅用于克隆源码）

> 依赖插件 `PoseSearch`、`Chooser`、`GameplayAbilities`、`AnimationLocomotionLibrary` 已在 `Alpha.uproject` 中声明。

---

## 快速开始

> ⚠️ 本仓库为**代码展示**用途，不包含 `Content/` 下的美术、蓝图、动画资产。
> 以下步骤可以完成编译，但打开编辑器后**没有可玩内容**（无地图、无角色资产），此为预期现象。

1. 安装 **Unreal Engine 5.8**。
2. 克隆仓库：
   ```bash
   git clone https://github.com/aidenzhang03-web/Alpha-Demo.git
   ```
3. 右键 `Alpha.uproject` → **Generate Visual Studio project files**。
4. 在 Visual Studio 中编译 `AlphaEditor` 目标（Development Editor / Win64）。
5. 双击 `Alpha.uproject` 打开编辑器（此时为空场景，属预期现象）。

---

## 操作说明

| 动作 | 按键 |
|---|---|
| 移动 | `W` `A` `S` `D` |
| 视角 | 鼠标 |
| 跳跃 | `Space` |
| 左键连招 | 鼠标左键 |
| 右键连招 | 鼠标右键 |
| 拔出/收起武器 | `R` |
| 上一把武器 | `Q` |
| 下一把武器 | `E` |
| 步行 | `左Alt` |
| 冲刺 | `左Shift` |

---

## 开发路线图

**已完成**
- [x] 三档位移动 + 惯性滑行刹车 + 折返转向
- [x] Motion Matching 移动动画系统
- [x] GAS 连招系统（基础 4 段 + 衍生 2 段）
- [x] 攻击命中判定 + GameplayEffect 伤害结算
- [x] 属性系统（生命 / 法力 / 耐力）
- [x] 敌人：血条 / 受击 / 死亡布娃娃 / 感知追击与攻击 AI
- [x] 多武器切换与收拔刀
- [x] 数据驱动脚步音效
- [x] 网络复制与服务器权威（Actor / 属性 / 能力）

**计划中**
- [ ] 玩家受击反馈与死亡逻辑（敌人攻击玩家已完成）
- [ ] 闪避 / 格挡技能
- [ ] 游戏流程完善（GameMode 胜负判定、重生）

---

## 说明

本项目用于学习与技术展示。仓库仅包含**源代码与工程配置**，不含任何美术 / 蓝图 / 动画资产（`.gitignore` 已整体排除 `Content/`）；代码中引用的第三方素材版权归原作者所有。
