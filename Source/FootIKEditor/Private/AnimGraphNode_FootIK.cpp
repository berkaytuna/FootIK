// Written by Berkay Tuna, June 2020

#include "AnimGraphNode_FootIK.h"
#include "FootIKEditor.h"
#include "AnimGraphNode_TwoBoneIK.h"
#include "..\Public\AnimGraphNode_FootIK.h"
#include "AnimNodeEditModes.h"

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_FootIK::UAnimGraphNode_FootIK()
{

}

FText UAnimGraphNode_FootIK::GetTooltipText() const
{
    return LOCTEXT("FootIK", "FootIK");
}

FText UAnimGraphNode_FootIK::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return LOCTEXT("FootIK", "FootIK");
}

FString UAnimGraphNode_FootIK::GetNodeCategory() const
{
    return TEXT("FootIK");
}

#undef LOCTEXT_NAMESPACE