// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/AlphaGameplayTags.h"

namespace AlphaGameplayTags
{
    // 这里把「C++ 变量名」和「实际的 Tag 字符串」绑定起来
    UE_DEFINE_GAMEPLAY_TAG(Input_Attack_Left, "Input.Attack.Left");
    UE_DEFINE_GAMEPLAY_TAG(Input_Attack_Right, "Input.Attack.Right");

    UE_DEFINE_GAMEPLAY_TAG(Combo_DerivedWindow, "Combo.DerivedWindow");
    UE_DEFINE_GAMEPLAY_TAG(Combo_DerivedWindowEnd, "Combo.DerivedWindowEnd");
    UE_DEFINE_GAMEPLAY_TAG(Combo_Window, "Combo.Window");
    UE_DEFINE_GAMEPLAY_TAG(Combo_WindowEnd, "Combo.WindowEnd");
    UE_DEFINE_GAMEPLAY_TAG(Combo_InterruptRequest, "Combo.InterruptRequest");
    UE_DEFINE_GAMEPLAY_TAG(Combo_BufferWindowOpen, "Combo.BufferWindowOpen");

    UE_DEFINE_GAMEPLAY_TAG(Ability_Combo, "Ability.Combo");
    UE_DEFINE_GAMEPLAY_TAG(State_Combo, "State.Combo");

    UE_DEFINE_GAMEPLAY_TAG(Attribute_HealthCost, "Attribute.HealthCost");
    UE_DEFINE_GAMEPLAY_TAG(Attribute_ManaCost, "Attribute.ManaCost");
    UE_DEFINE_GAMEPLAY_TAG(Attribute_StaminaCost, "Attribute.StaminaCost");
    UE_DEFINE_GAMEPLAY_TAG(Attribute_StaminaDrain, "Attribute.StaminaDrain");

    UE_DEFINE_GAMEPLAY_TAG(Ability_EnemyAttack, "Ability.EnemyAttack");
    UE_DEFINE_GAMEPLAY_TAG(State_Attacking, "State.Attacking");

    UE_DEFINE_GAMEPLAY_TAG(Event_HitReact, "Event.HitReact");
    UE_DEFINE_GAMEPLAY_TAG(Ability_HitReact, "Ability.HitReact");
    UE_DEFINE_GAMEPLAY_TAG(State_HitReact, "State.HitReact");
}