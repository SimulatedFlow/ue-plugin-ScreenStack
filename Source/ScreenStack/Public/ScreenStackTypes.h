// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ScreenStackTypes.generated.h"

/**
 * Where a layer belongs in the pile.
 *
 * The order is the point. Without it a push always lands on top, and the HUD that your inventory
 * screen pushed while a confirmation dialog was open takes the input away from the dialog.
 */
UENUM(BlueprintType)
enum class EScreenLayer : uint8
{
	/** Always-on game UI. Never owns input. */
	HUD,

	/** Full-screen menus: pause, inventory, map. */
	Menu,

	/** Something that must be answered before anything below is usable. */
	Modal,

	/** Small, transient, on top of whatever is open. */
	Popup,

	/** Above everything: a disconnect notice, a profile-change prompt. */
	System
};

/**
 * What a layer wants the input mode to be.
 *
 * `Inherit` is the important value: a layer with no opinion is transparent, and the answer comes
 * from whatever is beneath it. Most HUD layers should say Inherit.
 */
UENUM(BlueprintType)
enum class EScreenInputMode : uint8
{
	Inherit,
	Game,
	GameAndUI,
	UI
};

/** A yes/no a layer may decline to have. */
UENUM(BlueprintType)
enum class EScreenOpinion : uint8
{
	Inherit,
	Yes,
	No
};

/** One layer on the stack. */
USTRUCT(BlueprintType)
struct SCREENSTACK_API FScreenEntry
{
	GENERATED_BODY()

	/** The caller's handle - used to pop it again. Two entries may not share one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScreenStack")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScreenStack")
	EScreenLayer Layer = EScreenLayer::Menu;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScreenStack")
	EScreenInputMode InputMode = EScreenInputMode::Inherit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScreenStack")
	EScreenOpinion Cursor = EScreenOpinion::Inherit;

	/**
	 * Does this layer want the game paused?
	 *
	 * Yes is an assertion, not a toggle: while this layer is anywhere on the stack the game stays
	 * paused, whatever is pushed on top of it later.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScreenStack")
	EScreenOpinion Pause = EScreenOpinion::Inherit;

	/** Nothing below this layer is drawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScreenStack")
	bool bHidesBelow = false;

	/** Nothing below this layer may be interacted with. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScreenStack")
	bool bBlocksBelow = false;
};

/** The answer, recomputed from the whole stack after every change. */
USTRUCT(BlueprintType)
struct SCREENSTACK_API FScreenState
{
	GENERATED_BODY()

	/** Never Inherit - an empty stack resolves to Game. */
	UPROPERTY(BlueprintReadOnly, Category = "ScreenStack")
	EScreenInputMode InputMode = EScreenInputMode::Game;

	UPROPERTY(BlueprintReadOnly, Category = "ScreenStack")
	bool bCursor = false;

	UPROPERTY(BlueprintReadOnly, Category = "ScreenStack")
	bool bPaused = false;

	/** The topmost layer nothing above it blocks. None when the stack is empty. */
	UPROPERTY(BlueprintReadOnly, Category = "ScreenStack")
	FName TopInteractiveId;

	/** How many layers are actually drawn. */
	UPROPERTY(BlueprintReadOnly, Category = "ScreenStack")
	int32 VisibleCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "ScreenStack")
	int32 Depth = 0;
};
