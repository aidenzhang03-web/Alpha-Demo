// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h" 

// 项目的所有 GameplayTag 统一在这里登记（编译期检查，打错 Tag 名直接报错）
namespace AlphaGameplayTags
{
    // 输入
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Attack_Left);    // 左键攻击
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Attack_Right);   // 右键攻击

    // 连招
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo_DerivedWindow);  // 衍生窗口打开
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo_DerivedWindowEnd); // 衍生窗口关闭
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo_Window);        // 连招接续窗口打开
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo_WindowEnd);     // 连招接续窗口关闭
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo_InterruptRequest);  // 连招中断请求
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combo_BufferWindowOpen);  // 连招缓冲窗口打开

    // 能力标签
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Combo);        // 连招能力
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Combo);          // 连招进行中

    // 属性（SetByCaller 动态值）
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_HealthCost);      // 生命值消耗（伤害）量
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_ManaCost);    // 法力消耗量
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_StaminaCost); // 耐力消耗量
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_StaminaDrain);  // 持续扣耐力（每周期量）
}