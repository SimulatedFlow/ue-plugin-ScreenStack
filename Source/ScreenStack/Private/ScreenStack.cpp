// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "ScreenStack.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "ScreenStackLog.h"
#include "ScreenStackSubsystem.h"

DEFINE_LOG_CATEGORY(LogScreenStack);

#define LOCTEXT_NAMESPACE "FScreenStackModule"

namespace
{
	UWorld* ScreenStackConsoleWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game))
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	void ScreenStackDumpCommand()
	{
		UWorld* World = ScreenStackConsoleWorld();
		const UScreenStackSubsystem* Stack = World ? World->GetSubsystem<UScreenStackSubsystem>() : nullptr;
		if (!Stack)
		{
			UE_LOG(LogScreenStack, Warning, TEXT("ScreenStack.Dump: no running world."));
			return;
		}
		Stack->LogStack();
	}

	void ScreenStackPopCommand()
	{
		UWorld* World = ScreenStackConsoleWorld();
		UScreenStackSubsystem* Stack = World ? World->GetSubsystem<UScreenStackSubsystem>() : nullptr;
		if (!Stack)
		{
			UE_LOG(LogScreenStack, Warning, TEXT("ScreenStack.Pop: no running world."));
			return;
		}
		UE_LOG(LogScreenStack, Display, TEXT("ScreenStack.Pop: %s"),
			Stack->PopTop() ? TEXT("popped") : TEXT("nothing to pop"));
	}

	void ScreenStackClearCommand()
	{
		UWorld* World = ScreenStackConsoleWorld();
		UScreenStackSubsystem* Stack = World ? World->GetSubsystem<UScreenStackSubsystem>() : nullptr;
		if (!Stack)
		{
			UE_LOG(LogScreenStack, Warning, TEXT("ScreenStack.Clear: no running world."));
			return;
		}
		Stack->Clear();
		UE_LOG(LogScreenStack, Display, TEXT("ScreenStack.Clear: stack emptied."));
	}

	FAutoConsoleCommand GScreenStackDump(
		TEXT("ScreenStack.Dump"),
		TEXT("Log the stack top down, with the resolved input mode, cursor and pause."),
		FConsoleCommandDelegate::CreateStatic(&ScreenStackDumpCommand));

	FAutoConsoleCommand GScreenStackPop(
		TEXT("ScreenStack.Pop"),
		TEXT("Pop the top layer. The way out when a screen has trapped the input."),
		FConsoleCommandDelegate::CreateStatic(&ScreenStackPopCommand));

	FAutoConsoleCommand GScreenStackClear(
		TEXT("ScreenStack.Clear"),
		TEXT("Empty the stack and hand the input back to the game."),
		FConsoleCommandDelegate::CreateStatic(&ScreenStackClearCommand));
}

void FScreenStackModule::StartupModule()
{
}

void FScreenStackModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FScreenStackModule, ScreenStack)
