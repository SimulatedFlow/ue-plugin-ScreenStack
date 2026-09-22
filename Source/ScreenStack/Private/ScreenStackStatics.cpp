// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "ScreenStackStatics.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "ScreenStackSubsystem.h"

UScreenStackSubsystem* UScreenStackStatics::GetScreenStack(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	return World ? World->GetSubsystem<UScreenStackSubsystem>() : nullptr;
}

EScreenInputMode UScreenStackStatics::ResolveInputMode(const TArray<FScreenEntry>& Stack)
{
	// From the top down: the first layer with an opinion owns the input. Everything under it is
	// irrelevant, and everything above it that said Inherit has already been skipped.
	for (int32 i = Stack.Num() - 1; i >= 0; --i)
	{
		if (Stack[i].InputMode != EScreenInputMode::Inherit)
		{
			return Stack[i].InputMode;
		}
	}

	// An empty stack is the game. Not UI, and not "whatever it was last" - the second reading is how
	// a project ends up unable to move after closing a menu it never opened.
	return EScreenInputMode::Game;
}

bool UScreenStackStatics::ResolveCursor(const TArray<FScreenEntry>& Stack)
{
	for (int32 i = Stack.Num() - 1; i >= 0; --i)
	{
		if (Stack[i].Cursor != EScreenOpinion::Inherit)
		{
			return Stack[i].Cursor == EScreenOpinion::Yes;
		}
	}
	return false;
}

bool UScreenStackStatics::ResolvePause(const TArray<FScreenEntry>& Stack)
{
	// An OR over the WHOLE stack, not the topmost opinion.
	//
	// A popup with no opinion opening over a pause menu and then closing must not un-pause the game,
	// and that is exactly what "restore what the top layer wants" does. `No` is treated as no
	// opinion here on purpose: a layer saying "I do not need the pause" is not the same as one
	// saying "nobody may have it", and only the first is safe to act on.
	for (const FScreenEntry& E : Stack)
	{
		if (E.Pause == EScreenOpinion::Yes)
		{
			return true;
		}
	}
	return false;
}

int32 UScreenStackStatics::InsertIndexFor(EScreenLayer Layer, const TArray<FScreenEntry>& Stack)
{
	// After everything of the same layer or lower, before anything higher. A HUD pushed while a
	// modal is open therefore lands under it and steals nothing.
	const uint8 Neu = static_cast<uint8>(Layer);
	for (int32 i = 0; i < Stack.Num(); ++i)
	{
		if (static_cast<uint8>(Stack[i].Layer) > Neu)
		{
			return i;
		}
	}
	return Stack.Num();
}

bool UScreenStackStatics::IsVisible(int32 Index, const TArray<FScreenEntry>& Stack)
{
	if (!Stack.IsValidIndex(Index))
	{
		return false;
	}
	for (int32 i = Index + 1; i < Stack.Num(); ++i)
	{
		if (Stack[i].bHidesBelow)
		{
			return false;
		}
	}
	return true;
}

bool UScreenStackStatics::IsInteractive(int32 Index, const TArray<FScreenEntry>& Stack)
{
	if (!Stack.IsValidIndex(Index))
	{
		return false;
	}

	// Anything above that blocks ends it. A separate question from IsVisible: a modal that dims the
	// screen behind it still lets you see the inventory and must not let you click it.
	for (int32 i = Index + 1; i < Stack.Num(); ++i)
	{
		if (Stack[i].bBlocksBelow)
		{
			return false;
		}
	}
	return true;
}

FScreenState UScreenStackStatics::ResolveState(const TArray<FScreenEntry>& Stack)
{
	FScreenState S;
	S.Depth = Stack.Num();
	S.InputMode = ResolveInputMode(Stack);
	S.bCursor = ResolveCursor(Stack);
	S.bPaused = ResolvePause(Stack);

	S.TopInteractiveId = Stack.Num() > 0 ? Stack.Last().Id : NAME_None;

	for (int32 i = 0; i < Stack.Num(); ++i)
	{
		if (IsVisible(i, Stack))
		{
			++S.VisibleCount;
		}
	}
	return S;
}
