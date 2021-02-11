// Written by Berkay Tuna, June 2020

#pragma once

#include "CoreMinimal.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_FootIK.generated.h"

#pragma region classes_in_use
class USkeletalMeshComponent;
class UCapsuleComponent;
class UCharacterMovementComponent;
#pragma endregion

USTRUCT(BlueprintType)
struct FActorsToIgnore
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calculations")
	TArray<AActor*> ActorsToIgnore;
};

#pragma region variables
USTRUCT()
struct FPelvis
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = SkeletalControl)
	FBoneReference BoneReference;

	FVector Location;
	float Offset;
	FVector Translation;
};

USTRUCT()
struct FLeftFoot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = SkeletalControl)
	FBoneSocketTarget EffectorTarget;
	FVector EffectorLocation;
	UPROPERTY(EditAnywhere, Category = SkeletalControl)
	FName SocketName;
	UPROPERTY(EditAnywhere, Category = SkeletalControl)
	FBoneSocketTarget JointTarget;
	UPROPERTY(EditAnywhere, Category = SkeletalControl)
	FVector JointTargetLocation;

	FVector Location;
	float Offset;
	FRotator Rotation;

	// Line Trace
	bool Hit;
	FVector_NetQuantizeNormal HitNormal;

	// Correction
	float HitNormalMax;
};

USTRUCT()
struct FRightFoot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = SkeletalControl)
	FBoneSocketTarget EffectorTarget;
	FVector EffectorLocation;
	UPROPERTY(EditAnywhere, Category = SkeletalControl)
	FName SocketName;
	UPROPERTY(EditAnywhere, Category = SkeletalControl)
	FBoneSocketTarget JointTarget;
	UPROPERTY(EditAnywhere, Category = SkeletalControl)
	FVector JointTargetLocation;

	FVector Location;
	float Offset;
	FRotator Rotation;

	// Line Trace
	bool Hit;
	FVector_NetQuantizeNormal HitNormal;

	// Correction
	float HitNormalMax;
};
#pragma endregion

USTRUCT(BlueprintInternalUseOnly)
struct FOOTIK_API FAnimNode_FootIK : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

	USkeletalMeshComponent* Mesh;
	AActor* Actor;

	UCapsuleComponent* CapsuleComponent;
	UCharacterMovementComponent* CharacterMovement;

	UPROPERTY(EditAnywhere, Category = SkeletalControl)
	FPelvis Pelvis;
	UPROPERTY(EditAnywhere, Category = SkeletalControl)
	FLeftFoot LeftFoot;
	UPROPERTY(EditAnywhere, Category = SkeletalControl)
	FRightFoot RightFoot;

	// without default constructor
	FCompactPoseBoneIndex CachedLowerLimbIndex_L;
	FCompactPoseBoneIndex CachedUpperLimbIndex_L;

	FCompactPoseBoneIndex CachedLowerLimbIndex_R;
	FCompactPoseBoneIndex CachedUpperLimbIndex_R;

	// Calculations
	UPROPERTY(EditAnywhere, Category = "Calculations", meta = (PinShownByDefault))
	FActorsToIgnore ActorsToIgnore;

	UPROPERTY(EditAnywhere, Category = Calculations, meta = (ClampMin = "0.0", DisplayName = "Foot IK Apply Distance"))
	float ApplyDistance;

	UPROPERTY(EditAnywhere, Category = Calculations, meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "Offset Interpolation Alpha"))
	float OffsetAlpha;
	UPROPERTY(EditAnywhere, Category = Calculations, meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "Foot Rotation Interpolation Alpha"))
	float RotationAlpha;
	UPROPERTY(EditAnywhere, Category = Calculations, AdvancedDisplay, meta = (ClampMin = "0.5", ClampMax = "2.0", DisplayName = "Slope Correction Multiplier (Offset Interpolation Alpha)"))
	float SlopeMultiplier;
	
	UPROPERTY(EditAnywhere, Category = Calculations, AdvancedDisplay, meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "Sudden Slope Change During Sudden Height Change Compensation Alpha"))
	float SuddenSlopeChangeAlpha;
	float Delta_PelvisOffset_Normalized;

	FAnimNode_FootIK();

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

private:

	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

	// Line Trace
	void LineTraceCalculations(FVector_NetQuantizeNormal& HitNormal, float& PelvisOffset, USkeletalMeshComponent* Mesh, FVector PelvisLocation, FVector FootLocation, float CapsuleHalfHeight, float CapsuleRadius);
	
	// skeletal controls
	void PelvisSkeletalControl(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms, FBoneReference BoneReference, FBoneContainer BoneContainer, FVector Translation);
	void FootSkeletalControls(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms, FBoneReference BoneReference, FBoneContainer BoneContainer, FCompactPoseBoneIndex CachedUpperLimbIndex, FCompactPoseBoneIndex CachedLowerLimbIndex, FBoneSocketTarget EffectorTarget, FVector EffectorLocation, FBoneSocketTarget JointTarget, FVector JointTargetLocation, FRotator Rotation);
};