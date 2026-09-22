// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ScreenStackTypes.h"
#include "ScreenStackStatics.generated.h"

class UScreenStackSubsystem;

/**
 * The rules, on their own.
 *
 * Pure: no world, no widgets, no player controller. The subsystem calls exactly these and so do the
 * automation tests. Every one of them takes the WHOLE stack, because that is the fix - a state
 * derived from the stack on every change cannot drift out of step with it, and a state each widget
 * remembers for itself always does.
 */
UCLASS(meta = (ScriptName = "ScreenStackStatics"))
class SCREENSTACK_API UScreenStackStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** This world's stack. May be null outside a world. */
	UFUNCTION(BlueprintPure, Category = "ScreenStack", meta = (WorldContext = "WorldContextObject"))
	static UScreenStackSubsystem* GetScreenStack(const UObject* WorldContextObject);

	/** Everything at once. What the subsystem applies after every push and pop. */
	UFUNCTION(BlueprintPure, Category = "ScreenStack|Rules")
	static FScreenState ResolveState(const TArray<FScreenEntry>& Stack);

	/**
	 * The topmost layer that has an opinion about input.
	 *
	 * Layers set to Inherit are transparent: a HUD pushed over a menu does not hand input back to
	 * the game. An empty stack, or one where nobody has an opinion, is Game.
	 */
	UFUNCTION(BlueprintPure, Category = "ScreenStack|Rules")
	static EScreenInputMode ResolveInputMode(const TArray<FScreenEntry>& Stack);

	/** The same for the cursor. Nobody with an opinion means no cursor. */
	UFUNCTION(BlueprintPure, Category = "ScreenStack|Rules")
	static bool ResolveCursor(const TArray<FScreenEntry>& Stack);

	/**
	 * Paused if ANY layer on the stack wants it - not just the top one.
	 *
	 * This one rule is the fix for the bug every project ships: a popup opens over the pause menu,
	 * the popup has no opinion about pausing, the popup closes, and a "restore what the top layer
	 * wants" implementation un-pauses a game the menu underneath is still holding. An OR cannot do
	 * that.
	 *
	 * Note what that means for `No`: for pausing it is the same as Inherit. A layer cannot un-pause
	 * a game some other layer is holding, because "I do not need the pause" and "nobody may have the
	 * pause" are different statements and only the first one is safe to honour. A layer that really
	 * must have the game running should pop the one holding the pause. `No` still means No for the
	 * cursor and for the input mode, where the topmost opinion wins outright.
	 */
	UFUNCTION(BlueprintPure, Category = "ScreenStack|Rules")
	static bool ResolvePause(const TArray<FScreenEntry>& Stack);

	/**
	 * Where a push of this layer belongs.
	 *
	 * Sorted insert by layer order, after everything of the same layer. A HUD pushed while a modal
	 * is open lands underneath it and takes nothing away from it.
	 */
	UFUNCTION(BlueprintPure, Category = "ScreenStack|Rules")
	static int32 InsertIndexFor(EScreenLayer Layer, const TArray<FScreenEntry>& Stack);

	/** Is this layer drawn, or is something above it hiding it? */
	UFUNCTION(BlueprintPure, Category = "ScreenStack|Rules")
	static bool IsVisible(int32 Index, const TArray<FScreenEntry>& Stack);

	/**
	 * May this layer be interacted with?
	 *
	 * False as soon as anything above it blocks. This is what `bBlocksBelow` is for, and it is a
	 * different question from IsVisible: a modal that dims the screen behind it still lets you SEE
	 * the inventory underneath, and must not let you click it.
	 *
	 * The topmost layer is always interactive - something has to be.
	 */
	UFUNCTION(BlueprintPure, Category = "ScreenStack|Rules")
	static bool IsInteractive(int32 Index, const TArray<FScreenEntry>& Stack);
};
