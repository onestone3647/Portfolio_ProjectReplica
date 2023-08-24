// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ProjectReplica.h"
#include "GameFramework/Actor.h"
#include "PRPooledObject.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPooledObjectDeactivate, APRPooledObject*, PoolObject);

/**
 * 오브젝트 풀링에 사용하는 오브젝트 클래스입니다.
 */
UCLASS()
class PROJECTREPLICA_API APRPooledObject : public AActor
{
	GENERATED_BODY()
	
public:	
	APRPooledObject();

protected:
	virtual void BeginPlay() override;

public:
	/** 오브젝트가 활성화 되었는지 판별하는 함수입니다. */
	UFUNCTION(BlueprintCallable, Category = "Pooled Object")
	virtual bool IsActivate() const;

	/** 입력받은 인자로 오브젝트의 활성화를 설정하는 함수입니다. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Pooled Object")
	void SetActivate(bool bIsActivate);
	virtual void SetActivate_Implementation(bool bIsActivate);

	/** 오브젝트를 초기화하는 함수입니다. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Pooled Object")
	void InitializePooledObject();
	virtual void InitializePooledObject_Implementation();	

protected:
	/** 오브젝트를 비활성화하는 함수입니다. */
	UFUNCTION(BlueprintCallable, Category = "Pooled Object")
	virtual void Deactivate();

	/** 오브젝트의 Spawn 위치를 초기화하는 함수입니다. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Pooled Object")
	void InitializeSpawnLocation();
	virtual void InitializeSpawnLocation_Implementation();

protected:
	/** 오브젝트의 활성화를 나타내는 변수입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pooled Object")
	bool bActivate;
	
	/** 오브젝트의 소유자입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pooled Object")
	AActor* ObjectOwner;

	/** 오브젝트의 이름입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pooled Object")
	FName ObjectName;	

	/** 오브젝트의 수명입니다. 수명이 다할 경우 오브젝트는 비활성화됩니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pooled Object")
	float Lifespan;

	/** 오브젝트의 수명에 따라 오브젝트를 비활성화할 때 사용하는 TimerHandle입니다. */
	FTimerHandle LifespanTimerHandle;

	/** 풀의 Index입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pooled Object")
	int32 PoolIndex;

public:
	/** ObjectOwner를 반환하는 함수입니다. */
	AActor* GetObjectOwner() const;

	/** 입력받은 인자로 ObjectOwner를 설정하는 함수입니다. */
	void SetObjectOwner(AActor* NewObjectOwner);
	
	/** ObjectName을 반환하는 함수입니다. */
	FName GetObjectName() const;

	/** 입력받은 인자로 ObjectName을 설정하는 함수입니다. */
	void SetObjectName(FName NewObjectName);
	
	/** PoolIndex를 반환하는 함수입니다. */
	int32 GetPoolIndex() const;

	/** 입력받은 인자로 PoolIndex를 설정하는 함수입니다. */
	void SetPoolIndex(int32 NewPoolIndex);

	/** 입력받은 인자로 Lifespan을 설정하는 함수입니다. */
	void SetLifespan(float NewLifespan);

public:
	/** 오브젝트를 비활성화하는 델리게이트입니다. */
	FOnPooledObjectDeactivate OnPooledObjectDeactivate;
};