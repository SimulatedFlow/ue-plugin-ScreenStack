// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ScreenStackTypes.h"
#include "ScreenStackSubsystem.generated.h"

class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScreenStateChanged, const FScreenState&, State);

/**
 * The stack, and the one place that applies what it resolves to.
 *
 * Push and pop; everything else follows. The resolved state is applied to the player controller
 * only when it actually changed, so pushing a HUD layer over an open menu does not re-issue an
 * input mode the engine is already in.
 */
UCLASS()
class SCREENSTACK_API UScreenStackSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Put a layer on the stack. Ids are unique - pushing one that is already there replaces it in
	 * place rather than adding a second, because two layers with one handle cannot both be popped.
	 *
	 * @return Where it landed, which is not necessarily the top.
	 */
	UFUNCTION(BlueprintCallable, Category = "ScreenStack")
	int32 Push(const FScreenEntry& Entry);

	/** Take a layer off by its handle. Popping something that is not there is a no-op. */
	UFUNCTION(BlueprintCallable, Category = "ScreenStack")
	bool Pop(FName Id);

	/** Take the top layer off. */
	UFUNCTION(BlueprintCallable, Category = "ScreenStack")
	bool PopTop();

	/** Everything off. For a level change or a disconnect. */
	UFUNCTION(BlueprintCallable, Category = "ScreenStack")
	void Clear();

	UFUNCTION(BlueprintPure, Category = "ScreenStack")
	bool Contains(FName Id) const;

	UFUNCTION(BlueprintPure, Category = "ScreenStack")
	FScreenState GetState() const;

	UFUNCTION(BlueprintPure, Category = "ScreenStack")
	const TArray<FScreenEntry>& GetStack() const { return Stack; }

	/**
	 * Apply the resolved state to a player controller.
	 *
	 * Called after every change for the first local player. Exposed because a split-screen project
	 * has more than one, and a project with its own input plumbing may want to apply it elsewhere.
	 */
	UFUNCTION(BlueprintCallable, Category = "ScreenStack")
	void ApplyTo(APlayerController* Controller);

	/** Fires whenever the resolved state changes - not on every push. */
	UPROPERTY(BlueprintAssignable, Category = "ScreenStack")
	FOnScreenStateChanged OnScreenStateChanged;

	/** ScreenStack.Dump prints through this. */
	void LogStack() const;

	virtual void Deinitialize() override;

private:
	void AfterChange();

	UPROPERTY()
	TArray<FScreenEntry> Stack;

	FScreenState Letzter;
	bool bHatLetzten = false;
};
