// Written by Berkay Tuna, June 2020

#include "AnimNode_FootIK.h"
#include "AnimationRuntime.h"
#include "TwoBoneIK.h"
#include "Animation/AnimInstanceProxy.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"

DECLARE_CYCLE_STAT(TEXT("FootIK Eval"), STAT_FootIK_Eval, STATGROUP_Anim);

FAnimNode_FootIK::FAnimNode_FootIK()
:
	// Foot 
	  CachedLowerLimbIndex_L(INDEX_NONE)
	, CachedUpperLimbIndex_L(INDEX_NONE)

	, CachedLowerLimbIndex_R(INDEX_NONE)
	, CachedUpperLimbIndex_R(INDEX_NONE)

	// Calculations
	, ApplyDistance(60.0f)

	, OffsetAlpha(0.1f)
	, RotationAlpha(0.1f)
	, SlopeMultiplier(1.0f)
	, SuddenSlopeChangeAlpha(0.1f)
{
}

void FAnimNode_FootIK::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
	Super::Initialize_AnyThread(Context);

	// Foot
	LeftFoot.EffectorTarget.Initialize(Context.AnimInstanceProxy);
	LeftFoot.JointTarget.Initialize(Context.AnimInstanceProxy);

	RightFoot.EffectorTarget.Initialize(Context.AnimInstanceProxy);
	RightFoot.JointTarget.Initialize(Context.AnimInstanceProxy);

	// references
	Mesh = Context.AnimInstanceProxy->GetSkelMeshComponent();
	Actor = Mesh->GetOwner();

	CapsuleComponent = Actor->FindComponentByClass<UCapsuleComponent>();
	CharacterMovement = Actor->FindComponentByClass<UCharacterMovementComponent>();
}

void FAnimNode_FootIK::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(InitializeBoneReferences)
	
	// Pelvis
	Pelvis.BoneReference.Initialize(RequiredBones);

	// Foot
	LeftFoot.EffectorTarget.BoneReference.Initialize(RequiredBones);

	LeftFoot.EffectorTarget.InitializeBoneReferences(RequiredBones);
	LeftFoot.JointTarget.InitializeBoneReferences(RequiredBones);

	FCompactPoseBoneIndex IKBoneCompactPoseIndex_L = LeftFoot.EffectorTarget.BoneReference.GetCompactPoseIndex(RequiredBones);
	CachedLowerLimbIndex_L = FCompactPoseBoneIndex(INDEX_NONE);
	CachedUpperLimbIndex_L = FCompactPoseBoneIndex(INDEX_NONE);
	if (IKBoneCompactPoseIndex_L != INDEX_NONE)
	{
		CachedLowerLimbIndex_L = RequiredBones.GetParentBoneIndex(IKBoneCompactPoseIndex_L);
		if (CachedLowerLimbIndex_L != INDEX_NONE)
		{
			CachedUpperLimbIndex_L = RequiredBones.GetParentBoneIndex(CachedLowerLimbIndex_L);
		}
	}

	RightFoot.EffectorTarget.BoneReference.Initialize(RequiredBones);

	RightFoot.EffectorTarget.InitializeBoneReferences(RequiredBones);
	RightFoot.JointTarget.InitializeBoneReferences(RequiredBones);

	FCompactPoseBoneIndex IKBoneCompactPoseIndex_R = RightFoot.EffectorTarget.BoneReference.GetCompactPoseIndex(RequiredBones);
	CachedLowerLimbIndex_R = FCompactPoseBoneIndex(INDEX_NONE);
	CachedUpperLimbIndex_R = FCompactPoseBoneIndex(INDEX_NONE);
	if (IKBoneCompactPoseIndex_R != INDEX_NONE)
	{
		CachedLowerLimbIndex_R = RequiredBones.GetParentBoneIndex(IKBoneCompactPoseIndex_R);
		if (CachedLowerLimbIndex_R != INDEX_NONE)
		{
			CachedUpperLimbIndex_R = RequiredBones.GetParentBoneIndex(CachedLowerLimbIndex_R);
		}
	}
}

bool FAnimNode_FootIK::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	// Pelvis
	if (!Pelvis.BoneReference.IsValidToEvaluate(RequiredBones))
	{
		return false;
	}
	
	// Foot
	if ((!LeftFoot.EffectorTarget.BoneReference.IsValidToEvaluate(RequiredBones)) || (CachedUpperLimbIndex_L == INDEX_NONE) || (CachedLowerLimbIndex_L == INDEX_NONE) || (!LeftFoot.EffectorTarget.IsValidToEvaluate(RequiredBones)) || (!LeftFoot.JointTarget.IsValidToEvaluate(RequiredBones)))
	{
		return false;
	}

	if ((!RightFoot.EffectorTarget.BoneReference.IsValidToEvaluate(RequiredBones)) || (CachedUpperLimbIndex_R == INDEX_NONE) || (CachedLowerLimbIndex_R == INDEX_NONE) || (!RightFoot.EffectorTarget.IsValidToEvaluate(RequiredBones)) || (!RightFoot.JointTarget.IsValidToEvaluate(RequiredBones)))
	{
		return false;
	}

	return true;
}

void FAnimNode_FootIK::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{	
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(EvaluateSkeletalControl_AnyThread)
	SCOPE_CYCLE_COUNTER(STAT_FootIK_Eval);

	if (CapsuleComponent && CharacterMovement)
	{
		check(OutBoneTransforms.Num() == 0);

		const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

		const float CapsuleHalfHeight = CapsuleComponent->GetUnscaledCapsuleHalfHeight();
		const float CapsuleRadius = CapsuleComponent->GetUnscaledCapsuleRadius();

		TEnumAsByte<enum EMovementMode> MovementMode = CharacterMovement->MovementMode;

		FVector ActorLocation = Actor->GetActorLocation();
		LeftFoot.Location = Mesh->GetSocketLocation(LeftFoot.SocketName);
		RightFoot.Location = Mesh->GetSocketLocation(RightFoot.SocketName);

		float PelvisOffsetLeft;
		float PelvisOffsetRight;

		if (MovementMode == 1)
		{
		#pragma region calculations
			LineTraceCalculations(LeftFoot.HitNormal, PelvisOffsetLeft, Mesh, ActorLocation, LeftFoot.Location, CapsuleHalfHeight, CapsuleRadius);
			LineTraceCalculations(RightFoot.HitNormal, PelvisOffsetRight, Mesh, ActorLocation, RightFoot.Location, CapsuleHalfHeight, CapsuleRadius);

			// Offset
			LeftFoot.HitNormalMax = fabsf(LeftFoot.HitNormal.X) > fabsf(LeftFoot.HitNormal.Y) ? fabsf(LeftFoot.HitNormal.X) : fabsf(LeftFoot.HitNormal.Y);
			RightFoot.HitNormalMax = fabsf(RightFoot.HitNormal.X) > fabsf(RightFoot.HitNormal.Y) ? fabsf(RightFoot.HitNormal.X) : fabsf(RightFoot.HitNormal.Y);;
			float HitNormalMax_Min = LeftFoot.HitNormalMax < RightFoot.HitNormalMax ? LeftFoot.HitNormalMax : RightFoot.HitNormalMax;

			float PelvisOffsetNew = PelvisOffsetLeft < PelvisOffsetRight ? PelvisOffsetLeft : PelvisOffsetRight;

			float Delta_PelvisOffset_NormalizedNew = (fabsf(Pelvis.Offset) - fabsf(PelvisOffsetNew)) / ApplyDistance;

			if (Delta_PelvisOffset_NormalizedNew < Delta_PelvisOffset_Normalized)
			{
				Delta_PelvisOffset_Normalized = Delta_PelvisOffset_Normalized + SuddenSlopeChangeAlpha * (Delta_PelvisOffset_NormalizedNew - Delta_PelvisOffset_Normalized);
			}
			else
			{
				Delta_PelvisOffset_Normalized = Delta_PelvisOffset_NormalizedNew;
			}

			float HitNormalMax_Min_Corrected = SlopeMultiplier * HitNormalMax_Min;
			HitNormalMax_Min_Corrected = HitNormalMax_Min_Corrected > 1.0f ? 1.0f : HitNormalMax_Min_Corrected;

			float OffsetAlpha_Corrected = (HitNormalMax_Min_Corrected < OffsetAlpha ? OffsetAlpha : HitNormalMax_Min_Corrected) * (1.0f - Delta_PelvisOffset_Normalized);

			Pelvis.Offset = Pelvis.Offset + OffsetAlpha_Corrected * (PelvisOffsetNew - Pelvis.Offset);

			Pelvis.Translation = FVector(0.0f, 0.0f, Pelvis.Offset);

			if (PelvisOffsetLeft > PelvisOffsetRight)
			{
				LeftFoot.Offset = LeftFoot.Offset + OffsetAlpha_Corrected * (-PelvisOffsetRight + PelvisOffsetLeft - LeftFoot.Offset);

				RightFoot.Offset = RightFoot.Offset + OffsetAlpha_Corrected * (0.0f - RightFoot.Offset);
			}
			else
			{
				LeftFoot.Offset = LeftFoot.Offset + OffsetAlpha_Corrected * (0.0f - LeftFoot.Offset);

				RightFoot.Offset = RightFoot.Offset + OffsetAlpha_Corrected * (PelvisOffsetLeft - PelvisOffsetRight - RightFoot.Offset);
			}

			LeftFoot.EffectorLocation = FVector(LeftFoot.Offset, 0.0f, 0.0f);
			RightFoot.EffectorLocation = FVector(RightFoot.Offset, 0.0f, 0.0f);

			// Rotation
			LeftFoot.Rotation = LeftFoot.Rotation + RotationAlpha * (FRotator(-180.0f / PI * FGenericPlatformMath::Atan2(LeftFoot.HitNormal.X, LeftFoot.HitNormal.Z), 0.0f, 180.0f / PI * FGenericPlatformMath::Atan2(LeftFoot.HitNormal.Y, LeftFoot.HitNormal.Z)) - LeftFoot.Rotation);
			RightFoot.Rotation = RightFoot.Rotation + RotationAlpha * (FRotator(-180.0f / PI * FGenericPlatformMath::Atan2(RightFoot.HitNormal.X, RightFoot.HitNormal.Z), 0.0f, 180.0f / PI * FGenericPlatformMath::Atan2(RightFoot.HitNormal.Y, RightFoot.HitNormal.Z)) - RightFoot.Rotation);
		#pragma endregion
		}

		// skeletal controls
		// Pelvis translation
		PelvisSkeletalControl(Output, OutBoneTransforms, Pelvis.BoneReference, BoneContainer, Pelvis.Translation);

		// Foot ik and rotation
		FootSkeletalControls(Output, OutBoneTransforms, LeftFoot.EffectorTarget.BoneReference, BoneContainer, CachedUpperLimbIndex_L, CachedLowerLimbIndex_L, LeftFoot.EffectorTarget, LeftFoot.EffectorLocation, LeftFoot.JointTarget, LeftFoot.JointTargetLocation, LeftFoot.Rotation);
		FootSkeletalControls(Output, OutBoneTransforms, RightFoot.EffectorTarget.BoneReference, BoneContainer, CachedUpperLimbIndex_R, CachedLowerLimbIndex_R, RightFoot.EffectorTarget, RightFoot.EffectorLocation, RightFoot.JointTarget, RightFoot.JointTargetLocation, RightFoot.Rotation);

		// Make sure we have correct number of bones
		check(OutBoneTransforms.Num() == 7);
	}
}

void FAnimNode_FootIK::LineTraceCalculations(FVector_NetQuantizeNormal& HitNormal, float& PelvisOffset, USkeletalMeshComponent* mesh, FVector ActorLocation, FVector FootLocation, float CapsuleHalfHeight, float CapsuleRadius)
{
	FHitResult HitResult;
	FCollisionQueryParams TraceParams;

	FVector Start = FVector(FootLocation.X, FootLocation.Y, ActorLocation.Z);
	FVector End = FVector(FootLocation.X, FootLocation.Y, (ActorLocation.Z - CapsuleHalfHeight - ApplyDistance));

	ActorsToIgnore.ActorsToIgnore.Add(Actor);

	UKismetSystemLibrary::LineTraceSingle(Mesh, Start, End, TraceTypeQuery1, false, ActorsToIgnore.ActorsToIgnore, EDrawDebugTrace::None, HitResult, true);

	HitNormal = HitResult.Normal;

	// Pelvis Offset
	if (HitResult.bBlockingHit)
	{
		PelvisOffset = CapsuleHalfHeight - HitResult.Distance;
	}
	else
	{
		PelvisOffset = 0.0f;
	}
}

void FAnimNode_FootIK::PelvisSkeletalControl(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms, FBoneReference BoneReference, FBoneContainer BoneContainer, FVector Translation)
{
	FCompactPoseBoneIndex CompactPoseBoneReference = BoneReference.GetCompactPoseIndex(BoneContainer);
	FTransform NewBoneTM = Output.Pose.GetComponentSpaceTransform(CompactPoseBoneReference);
	FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();

	FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneReference, BCS_ComponentSpace);
	NewBoneTM.AddToTranslation(Translation);
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneReference, BCS_ComponentSpace);

	OutBoneTransforms.Add(FBoneTransform(BoneReference.GetCompactPoseIndex(BoneContainer), NewBoneTM));
}

void FAnimNode_FootIK::FootSkeletalControls(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms, FBoneReference BoneReference, FBoneContainer BoneContainer, FCompactPoseBoneIndex CachedUpperLimbIndex, FCompactPoseBoneIndex CachedLowerLimbIndex, FBoneSocketTarget EffectorTarget, FVector EffectorLocation, FBoneSocketTarget JointTarget, FVector JointTargetLocation, FRotator Rotation)
{
	// TwoBone IK
	// Get indices of the lower and upper limb bones and check validity.
	FCompactPoseBoneIndex BoneReferenceCompactPoseIndex = BoneReference.GetCompactPoseIndex(BoneContainer);

	// Get Component Space transforms for our bones.
	FTransform UpperLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedUpperLimbIndex);
	FTransform LowerLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex);
	FTransform EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(BoneReferenceCompactPoseIndex);

	// This is our reach goal.
	FVector DesiredPos = ((EffectorTarget.GetTargetTransform(EffectorLocation, Output.Pose, Output.AnimInstanceProxy->GetComponentTransform())).GetTranslation() + Pelvis.Translation);
	FVector	JointTargetPos = (JointTarget.GetTargetTransform(JointTargetLocation, Output.Pose, Output.AnimInstanceProxy->GetComponentTransform())).GetTranslation();

	UpperLimbCSTransform.SetLocation(UpperLimbCSTransform.GetTranslation() + Pelvis.Translation);
	LowerLimbCSTransform.SetLocation(LowerLimbCSTransform.GetTranslation() + Pelvis.Translation);
	EndBoneCSTransform.SetLocation(EndBoneCSTransform.GetTranslation() + Pelvis.Translation);

	AnimationCore::SolveTwoBoneIK(UpperLimbCSTransform, LowerLimbCSTransform, EndBoneCSTransform, JointTargetPos, DesiredPos, false, 1.0f, 1.2f);

	// Rotation
	FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
	FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, EndBoneCSTransform, BoneReferenceCompactPoseIndex, BCS_WorldSpace);
	const FQuat BoneQuat(Rotation);
	EndBoneCSTransform.SetRotation(BoneQuat * EndBoneCSTransform.GetRotation());
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, EndBoneCSTransform, BoneReferenceCompactPoseIndex, BCS_WorldSpace);

	// Update transforms. Order is important.
	OutBoneTransforms.Add(FBoneTransform(CachedUpperLimbIndex, UpperLimbCSTransform));
	OutBoneTransforms.Add(FBoneTransform(CachedLowerLimbIndex, LowerLimbCSTransform));
	OutBoneTransforms.Add(FBoneTransform(BoneReferenceCompactPoseIndex, EndBoneCSTransform));
}