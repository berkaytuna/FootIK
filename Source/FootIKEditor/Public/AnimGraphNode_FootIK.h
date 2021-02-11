// Written by Berkay Tuna, June 2020

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_SkeletalControlBase.h"
#include "AnimNode_FootIK.h"
#include "AnimGraphNode_FootIK.generated.h"

struct FAnimNode_FootIK;
/**
 *
 */
UCLASS()
class FOOTIKEDITOR_API UAnimGraphNode_FootIK : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, Category = "Settings")
    FAnimNode_FootIK Node;

    //~ Begin UEdGraphNode Interface.
    virtual FText GetTooltipText() const override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    //~ End UEdGraphNode Interface.

    //~ Begin UAnimGraphNode_Base Interface
	virtual FString GetNodeCategory() const override;
    //~ End UAnimGraphNode_Base Interface

	// UAnimGraphNode_SkeletalControlBase interface
	virtual const FAnimNode_SkeletalControlBase* GetNode() const override { return &Node; }
	// End of UAnimGraphNode_SkeletalControlBase interface

    UAnimGraphNode_FootIK();
};