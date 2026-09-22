// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "ScreenStackSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "ScreenStackLog.h"
#include "ScreenStackStatics.h"

namespace ScreenStackLocal
{
	static bool Gleich(const FScreenState& A, const FScreenState& B)
	{
		return A.InputMode == B.InputMode
			&& A.bCursor == B.bCursor
			&& A.bPaused == B.bPaused
			&& A.TopInteractiveId == B.TopInteractiveId
			&& A.VisibleCount == B.VisibleCount
			&& A.Depth == B.Depth;
	}
}

int32 UScreenStackSubsystem::Push(const FScreenEntry& Entry)
{
	// A handle that is already on the stack replaces its entry where it stands. Adding a second one
	// would leave a layer nobody can pop - Pop takes the first match and the other stays forever.
	const int32 Vorhanden = Stack.IndexOfByPredicate(
		[&Entry](const FScreenEntry& E) { return !E.Id.IsNone() && E.Id == Entry.Id; });
	if (Vorhanden != INDEX_NONE)
	{
		Stack[Vorhanden] = Entry;
		AfterChange();
		return Vorhanden;
	}

	const int32 Wohin = UScreenStackStatics::InsertIndexFor(Entry.Layer, Stack);
	Stack.Insert(Entry, Wohin);
	AfterChange();
	return Wohin;
}

bool UScreenStackSubsystem::Pop(FName Id)
{
	const int32 Index = Stack.IndexOfByPredicate(
		[Id](const FScreenEntry& E) { return E.Id == Id; });
	if (Index == INDEX_NONE)
	{
		return false;
	}
	Stack.RemoveAt(Index, 1, EAllowShrinking::No);
	AfterChange();
	return true;
}

bool UScreenStackSubsystem::PopTop()
{
	if (Stack.Num() == 0)
	{
		// A no-op, not a warning. Closing a screen that is already closed happens on every double
		// click of a back button, and it is not worth a log line.
		return false;
	}
	Stack.Pop(EAllowShrinking::No);
	AfterChange();
	return true;
}

void UScreenStackSubsystem::Clear()
{
	if (Stack.Num() == 0)
	{
		return;
	}
	Stack.Reset();
	AfterChange();
}

bool UScreenStackSubsystem::Contains(FName Id) const
{
	return Stack.ContainsByPredicate([Id](const FScreenEntry& E) { return E.Id == Id; });
}

FScreenState UScreenStackSubsystem::GetState() const
{
	return UScreenStackStatics::ResolveState(Stack);
}

void UScreenStackSubsystem::AfterChange()
{
	const FScreenState Neu = GetState();
	if (bHatLetzten && ScreenStackLocal::Gleich(Neu, Letzter))
	{
		return;
	}
	Letzter = Neu;
	bHatLetzten = true;

	if (UWorld* World = GetWorld())
	{
		ApplyTo(UGameplayStatics::GetPlayerController(World, 0));
	}
	OnScreenStateChanged.Broadcast(Neu);
}

void UScreenStackSubsystem::ApplyTo(APlayerController* Controller)
{
	if (!Controller)
	{
		return;
	}

	const FScreenState S = GetState();

	switch (S.InputMode)
	{
	case EScreenInputMode::UI:
	{
		FInputModeUIOnly Modus;
		Modus.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Controller->SetInputMode(Modus);
		break;
	}
	case EScreenInputMode::GameAndUI:
	{
		FInputModeGameAndUI Modus;
		Modus.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Modus.SetHideCursorDuringCapture(false);
		Controller->SetInputMode(Modus);
		break;
	}
	default:
		Controller->SetInputMode(FInputModeGameOnly());
		break;
	}

	Controller->bShowMouseCursor = S.bCursor;

	// Pause goes through the controller rather than the world so that a project using a game mode
	// that refuses pausing keeps refusing it - the plugin states what the UI wants, it does not
	// overrule the rules of the game.
	Controller->SetPause(S.bPaused);
}

void UScreenStackSubsystem::LogStack() const
{
	const FScreenState S = GetState();
	UE_LOG(LogScreenStack, Display,
		TEXT("ScreenStack: %d layer(s), input %s, cursor %s, paused %s, top '%s'"),
		S.Depth, *UEnum::GetValueAsString(S.InputMode),
		S.bCursor ? TEXT("on") : TEXT("off"), S.bPaused ? TEXT("yes") : TEXT("no"),
		*S.TopInteractiveId.ToString());

	for (int32 i = Stack.Num() - 1; i >= 0; --i)
	{
		const FScreenEntry& E = Stack[i];
		UE_LOG(LogScreenStack, Display, TEXT("  [%d] %-10s %-14s input %-10s %s%s%s"),
			i, *UEnum::GetValueAsString(E.Layer), *E.Id.ToString(),
			*UEnum::GetValueAsString(E.InputMode),
			UScreenStackStatics::IsVisible(i, Stack) ? TEXT("") : TEXT("hidden "),
			UScreenStackStatics::IsInteractive(i, Stack) ? TEXT("") : TEXT("blocked "),
			E.Pause == EScreenOpinion::Yes ? TEXT("pauses") : TEXT(""));
	}
}

void UScreenStackSubsystem::Deinitialize()
{
	Stack.Reset();
	Super::Deinitialize();
}
