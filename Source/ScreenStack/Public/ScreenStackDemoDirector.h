// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScreenStackTypes.h"
#include "ScreenStackDemoDirector.generated.h"

class UTextRenderComponent;

/**
 * Runs the shipped demo: a stack being built and taken apart, with the resolved state beside it.
 *
 * A UI stack has nothing to film, so the demo draws the stack itself - one bar per layer, bottom to
 * top - and the three numbers it resolves to. The script is built around the two beats that go
 * wrong elsewhere: a HUD element pushed while a modal is open lands UNDER it and steals nothing,
 * and a popup closing over a pause menu leaves the game paused.
 *
 * It drives the real UScreenStackSubsystem. In an editor viewport there is no player controller, so
 * nothing is actually applied - the resolved state on the board is still the subsystem's own.
 */
UCLASS(meta = (DisplayName = "ScreenStack Demo Director"))
class SCREENSTACK_API AScreenStackDemoDirector : public AActor
{
	GENERATED_BODY()

public:
	AScreenStackDemoDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScreenStack Demo",
		meta = (ClampMin = "16.0", ClampMax = "180.0"))
	float CycleSeconds = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScreenStack Demo")
	bool bDrawDemo = true;

	/** The headline. TextRender, because HighResShot does not capture DrawDebugString. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ScreenStack Demo")
	TObjectPtr<UTextRenderComponent> BoardText;

	/** The stack, top line first. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ScreenStack Demo")
	TObjectPtr<UTextRenderComponent> StackText;

	/** What it resolves to. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ScreenStack Demo")
	TObjectPtr<UTextRenderComponent> StateText;

private:
	struct FSchritt
	{
		float Wann = 0.0f;
		bool bPush = false;
		FScreenEntry Eintrag;   // for a push
		FName Id;               // for a pop; None means clear
		FString Text;
	};

	void StartCycle();
	FString BuildStack() const;
	FString BuildState() const;
	void DrawStack() const;

	TArray<FSchritt> Skript;
	int32 Naechster = 0;
	FString Letzte;
	float CycleTime = 0.0f;
	bool bBereit = false;
};
