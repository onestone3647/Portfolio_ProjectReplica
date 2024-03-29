// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/PRBaseCharacter.h"
#include "ProjectReplicaGameMode.h"
#include "Characters/PRAICharacter.h"
#include "Characters/PRPlayerCharacter.h"
#include "Components/PRStatSystemComponent.h"
#include "Components/PRDamageSystemComponent.h"
#include "Components/PRAnimSystemComponent.h"
#include "Components/PRMovementSystemComponent.h"
#include "Components/PRStateSystemComponent.h"
#include "Components/PRWeaponSystemComponent.h"
#include "Components/PRSkillSystemComponent.h"
#include "Components/PRObjectPoolSystemComponent.h"
#include "Components/PREffectSystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PRTimeStopSystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

APRBaseCharacter::APRBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// CapsuleComponent
	GetCapsuleComponent()->InitCapsuleSize(30.0f, 94.0f);

	// Pawn
	// 컨트롤러가 회전할 때 캐리겉가 같이 회전하지 않도록 설정합니다.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// SkeletalMesh
	// 기본 스켈레탈 메시를 설정합니다.
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -96.0f), FRotator(0.0f, 270.0f, 0.0f));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SK_Mannequin(TEXT("/Game/Characters/Mannequin/Character/Mesh/SK_Mannequin"));
	if(SK_Mannequin.Succeeded() == true)
	{
		GetMesh()->SetSkeletalMesh(SK_Mannequin.Object);
	}

	// TakeDamage
	HitAnimMontage = FPRAnimMontage();
	
	// MovementInput
	bJumped = false;
	
	// StatSystem
	StatSystem = CreateDefaultSubobject<UPRStatSystemComponent>(TEXT("StatSystem"));

	// DamageSystem
	DamageSystem = CreateDefaultSubobject<UPRDamageSystemComponent>(TEXT("DamageSystem"));

	// AnimSystem
	AnimSystem = CreateDefaultSubobject<UPRAnimSystemComponent>(TEXT("AnimSystem"));

	// MovementSystem
	MovementSystem = CreateDefaultSubobject<UPRMovementSystemComponent>(TEXT("MovementSystem"));

	// StateSystem
	StateSystem = CreateDefaultSubobject<UPRStateSystemComponent>(TEXT("StateSystem"));

	// WeaponSystem
	WeaponSystem = CreateDefaultSubobject<UPRWeaponSystemComponent>(TEXT("WeaponSystem"));

	// EffectSystem
	EffectSystem = CreateDefaultSubobject<UPREffectSystemComponent>(TEXT("EffectSystem"));

	// ObjectPoolSystem
	ObjectPoolSystem = CreateDefaultSubobject<UPRObjectPoolSystemComponent>(TEXT("ObjectPoolSystem"));
	
	// SkillSystem
	SkillSystem = CreateDefaultSubobject<UPRSkillSystemComponent>(TEXT("SkillSystem"));

	// CharacterMovement
	GetCharacterMovement()->bOrientRotationToMovement = true;								// 캐릭터가 이동하는 방향으로 회전합니다.
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);	// 캐릭터의 회전속도입니다.
	GetCharacterMovement()->GravityScale = 2.0f;
	GetCharacterMovement()->JumpZVelocity = 800.0f;
	GetCharacterMovement()->AirControl = 0.15f;
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 180.0f;
	GetCharacterMovement()->BrakingFrictionFactor = 1.0f;		// 기본 값은 2.0f입니다. MovementSystem의 PredictStopLocation을 사용하기 위해서 1.0f으로 설정합니다.

	// Dodge
	DodgePRAnimMontages.Empty();
	ForwardDodgePRAnimMontageID = 0;
	BackwardDodgePRAnimMontageID = 0;
	LeftDodgePRAnimMontageID = 0;
	RightDodgePRAnimMontageID = 0;
	AerialForwardDodgePRAnimMontageID = 0;
	
	// Effect
	SignatureEffectColor = FLinearColor(20.0f, 15.0f, 200.0f, 1.0f);
}

void APRBaseCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// DamageSystem
	GetDamageSystem()->OnDeathDelegate.AddDynamic(this, &APRBaseCharacter::Death);
	GetDamageSystem()->OnBlockedDelegate.AddDynamic(this, &APRBaseCharacter::Blocked);
	GetDamageSystem()->OnDamageResponseDelegate.AddDynamic(this, &APRBaseCharacter::DamageResponse);
	
	// StatSystem
	// GetStatSystem()->OnHealthPointIsZeroDelegate.AddUFunction(this, FName("Dead"));

	// ObjectPool 초기화
	GetObjectPoolSystem()->InitializeObjectPool();
	
	// EffectPool 초기화
	GetEffectSystem()->InitializeEffectPool();

	// WeaponSystem 초기화
	GetWeaponSystem()->InitializeWeaponInventory();
}

void APRBaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetMovementSystem()->SetAllowedGait(EPRGait::Gait_Running);
	GetMovementSystem()->SetRotationMode(EPRRotationMode::RotationMode_VelocityDirection);

	// Dodge
	InitializeDodgePRAnimMontages();
}

void APRBaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APRBaseCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	GetMovementSystem()->OnCharacterMovementModeChanged(GetCharacterMovement()->MovementMode);
}

void APRBaseCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if(GetWorld()->GetTimerManager().IsTimerActive(JumpedDelayTimerHandle) == true)
	{
		GetWorld()->GetTimerManager().ClearTimer(JumpedDelayTimerHandle);
	}
	
	bJumped = false;
}

float APRBaseCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float FinalDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	PR_LOG_SCREEN("TakeDamage, ApplyDamage -> 인터페이스 TakeDamage로 수정바람");

	// 캐릭터가 죽지않았을 경우 피격을 적용합니다.
	if(GetStateSystem()->IsDead() == false && GetStateSystem()->IsInvincible() == false)
	{
		// 대미지 커서가 AI일 경우
		TakeHit(DamageCauser);
		GetStatSystem()->TakeDamage(FinalDamage);
	}
	
	return FinalDamage;
}

#pragma region Interface_Damageable
float APRBaseCharacter::GetCurrentHealth_Implementation()
{
	// if(GetDamageSystem() != nullptr)
	// {
	// 	return GetDamageSystem()->GetHealth();
	// }

	if(GetStatSystem() != nullptr)
	{
		return GetStatSystem()->GetCharacterStat().HealthPoint;
	}
	
	return 0.0f;
}

float APRBaseCharacter::GetMaxHealth_Implementation()
{
	// if(GetDamageSystem() != nullptr)
	// {
	// 	return GetDamageSystem()->GetMaxHealth();
	// }

	if(GetStatSystem() != nullptr)
	{
		return GetStatSystem()->GetCharacterStat().MaxHealthPoint;
	}
	
	return 0.0f;
}

float APRBaseCharacter::Heal_Implementation(float Amount)
{
	if(GetDamageSystem() != nullptr)
	{
		return GetDamageSystem()->Heal(Amount);
	}
	
	return 0.0f;
}

bool APRBaseCharacter::TakeDamage_Implementation(FPRDamageInfo DamageInfo)
{
	if(GetDamageSystem() != nullptr)
	{
		return GetDamageSystem()->TakeDamage(DamageInfo);
	}
	
	return false;
}
#pragma endregion 

#pragma region TakeDamage
bool APRBaseCharacter::IsDead() const
{
	return GetStateSystem()->IsDead();
}

bool APRBaseCharacter::IsInvincible() const
{
	return GetStateSystem()->IsInvincible();
}

void APRBaseCharacter::TakeHit(AActor* DamageCauser)
{
	StateSystem->SetIsHit(true);

	// 대미지 커서가 플레이어 캐릭터일 경우
	APRPlayerCharacter* PRPlayerCharacter = Cast<APRPlayerCharacter>(DamageCauser);
	if(IsValid(PRPlayerCharacter) == true)
	{
		// TimeStop 상태가 아닌 경우 대미지를 준 액터를 바라봅니다.
		// if(PRPlayerCharacter->GetTimeStopSystem()->IsActivateTimeStop() == false)
		// {
		// 	const FVector PRPlayerCharacterReverseForwardVector = PRPlayerCharacter->GetActorForwardVector() * -1.0f;
		// 	const FRotator LookAtRotation = UKismetMathLibrary::MakeRotFromX(PRPlayerCharacterReverseForwardVector);
		// 	SetActorRotation(LookAtRotation);
		// 	PlayAnimMontage(HitAnimMontage.AnimMontage);
		// }
		AProjectReplicaGameMode* PRGameMode = Cast<AProjectReplicaGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
		if(IsValid(PRGameMode) && !PRGameMode->GetTimeStopSystem()->IsActivateTimeStop())
		{
			const FVector PRPlayerCharacterReverseForwardVector = PRPlayerCharacter->GetActorForwardVector() * -1.0f;
			const FRotator LookAtRotation = UKismetMathLibrary::MakeRotFromX(PRPlayerCharacterReverseForwardVector);
			SetActorRotation(LookAtRotation);
			PlayAnimMontage(HitAnimMontage.AnimMontage);
		}

		// 플레이어 캐릭터의 ComboCount를 실행합니다.
		PRPlayerCharacter->ActivateComboCount();
	}
	else
	{
		// 대미지 커서가 AI일 경우
		APRAICharacter* PRAICharacter = Cast<APRAICharacter>(DamageCauser);
		if(IsValid(PRAICharacter) == true)
		{
			// 대미지를 준 액터를 바라봅니다.
			const FVector PRAICharacterReverseForwardVector = PRAICharacter->GetActorForwardVector() * -1.0f;
			const FRotator LookAtRotation = UKismetMathLibrary::MakeRotFromX(PRAICharacterReverseForwardVector);
			SetActorRotation(LookAtRotation);
			PlayAnimMontage(HitAnimMontage.AnimMontage);
		}
	}
}

void APRBaseCharacter::Dead()
{
	GetStateSystem()->SetIsDead(true);
	SetActorEnableCollision(false);
}
#pragma endregion 

#pragma region MovementInput
bool APRBaseCharacter::IsJumped() const
{
	return bJumped;
}

void APRBaseCharacter::Jump()
{
	Super::Jump();
	
	if(GetStateSystem()->IsActionable(EPRAction::Action_Move) == true && GetMovementSystem()->IsEqualMovementState(EPRMovementState::MovementState_InAir) == false)
	{
		bJumped = true;
	
		if(GetWorld()->GetTimerManager().IsTimerActive(JumpedDelayTimerHandle) == false)
		{
			GetWorld()->GetTimerManager().SetTimer(JumpedDelayTimerHandle, FTimerDelegate::CreateLambda([&]()
			{
				bJumped = false;
	
				GetWorld()->GetTimerManager().ClearTimer(JumpedDelayTimerHandle);
			}), 0.1f, false);
		}
	}
}
#pragma endregion

#pragma region DamageSystem
void APRBaseCharacter::Death()
{
	// GetMesh()->SetSimulatePhysics(true);
	// GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	
	GetStateSystem()->SetIsDead(true);
	SetActorEnableCollision(false);
}

void APRBaseCharacter::Blocked(bool bCanBeParried)
{
	PR_LOG_SCREEN("Blocked");
}

void APRBaseCharacter::DamageResponse(EPRDamageResponse DamageResponse)
{
	PR_LOG_SCREEN("DamageResponse: %s", *PRCommonEnum::GetEnumDisplayNameToString(TEXT("EPRDamageResponse"), static_cast<uint8>(DamageResponse)));
}

void APRBaseCharacter::DoDamage()
{
	if(IsDead())
	{
		return;
	}
	
	TArray<FHitResult> HitResults;
	bool bIsHit = false;
	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = TraceStart + GetActorForwardVector() * 100.0f;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	
	// ObjectTypeQuery3 = ObjectType Pawn
	ObjectTypes.Emplace(EObjectTypeQuery::ObjectTypeQuery3);
			
	// 자신을 Trace 대상에서 제외합니다.
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Emplace(this);

	// Debug 실행을 설정합니다.
	EDrawDebugTrace::Type DebugType = EDrawDebugTrace::None;
	if(bDamageSystemDebug)
	{
		DebugType = EDrawDebugTrace::ForDuration;
	}

	TSet<AActor*> UniqueActors;

	bIsHit = UKismetSystemLibrary::SphereTraceMultiForObjects(GetWorld(), TraceStart, TraceEnd, 20.0f, ObjectTypes, false, ActorsToIgnore, DebugType, HitResults, true);
	if(bIsHit)
	{
		for(FHitResult HitResult : HitResults)
		{
			if(HitResult.Actor.IsValid()
				&& HitResult.Actor->GetClass()->ImplementsInterface(UInterface_PRDamageable::StaticClass())
				&& UniqueActors.Find(HitResult.GetActor()) == nullptr)
			{
				PR_LOG_SCREEN("Hit Actor Name: %s", *HitResult.GetActor()->GetName());
				
				UniqueActors.Emplace(HitResult.GetActor());
				FPRDamageInfo DamageInfo;
				DamageInfo.Amount = 10.0f;
				DamageInfo.DamageType = EPRDamageType::DamageType_Melee;
				DamageInfo.DamageResponse = EPRDamageResponse::DamageResponse_HitReaction;
				
				bool bWasDamaged = IInterface_PRDamageable::Execute_TakeDamage(HitResult.Actor.Get(), DamageInfo);
				if(bWasDamaged)
				{
					GetEffectSystem()->SpawnNiagaraEffectAtLocation(HitEffect,HitResult.Location);
				}
			}
		}
	}
	
	// Hit된 액터들에게 대미지를 줍니다.
	// if(bIsHit)
	// {
	// 	for(FHitResult Hit : HitResults)
	// 	{
	// 		if(Hit.Actor.IsValid() == true && IsHitActor(NewHitActors, *Hit.GetActor()) == false)
	// 		{
	// 			AActor* HitActor = Hit.GetActor();
	// 			NewHitActors.Emplace(HitActor, false);
	//
	// 			if(IsTakeDamageActor(NewHitActors, *HitActor) == false)
	// 			{
	// 				ApplyDamage(NewHitActors, HitActor);
	// 				SpawnHitEffectByWeaponPosition(NewWeaponPosition, Hit.ImpactPoint);
	// 			}
	// 		}
	// 	}
	// }
}
#pragma endregion 

#pragma region AnimSystem
void APRBaseCharacter::InitializePRAnimMontages()
{
}
#pragma endregion 

#pragma region WeaponSystem
APRBaseWeapon* APRBaseCharacter::DrawEquippedWeapon()
{
	if(GetWeaponSystem()->IsValidWeaponIndex(GetWeaponSystem()->GetEquippedWeaponIndex()) == true)
	{
		int32 EquippedWeaponIndex = GetWeaponSystem()->GetEquippedWeaponIndex();

		return GetWeaponSystem()->DrawWeapon(EquippedWeaponIndex);
	}

	return nullptr;
}

APRBaseWeapon* APRBaseCharacter::SheathEquippedWeapon()
{
	if(GetWeaponSystem()->IsValidWeaponIndex(GetWeaponSystem()->GetEquippedWeaponIndex()) == true)
	{
		int32 EquippedWeaponIndex = GetWeaponSystem()->GetEquippedWeaponIndex();

		return GetWeaponSystem()->SheathWeapon(EquippedWeaponIndex);
	}

	return nullptr;
}
#pragma endregion 

#pragma region SkillSystem
UPRBaseSkill* APRBaseCharacter::GetSkillFromCommand(EPRCommandSkill NewCommandSkill) const
{
	return GetSkillSystem()->GetSkillFromCommand(NewCommandSkill);
}
#pragma endregion 

#pragma region Dodge
void APRBaseCharacter::InitializeDodgePRAnimMontages()
{
	DodgePRAnimMontages = GetAnimSystem()->GetPRAnimMontageFromPRAnimMontageDataTableByIDRangeToMap(ForwardDodgePRAnimMontageID, AerialForwardDodgePRAnimMontageID);
}

void APRBaseCharacter::Dodge()
{
	// 하위 클래스에서 오버라이딩합니다.
}
#pragma endregion 

#pragma region NormalAttack
void APRBaseCharacter::InitializePlayNormalAttackIndex()
{
}

void APRBaseCharacter::IncreasePlayNormalAttackIndex()
{
}
#pragma endregion

#pragma region Effect
FLinearColor APRBaseCharacter::GetSignatureEffectColor() const
{
	return SignatureEffectColor;
}
#pragma endregion 

