// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "ScreenStackDemoDirector.h"

#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "ScreenStackStatics.h"
#include "ScreenStackSubsystem.h"

namespace ScreenStackDemoLocal
{
	static FColor LayerColour(EScreenLayer L)
	{
		switch (L)
		{
		case EScreenLayer::HUD:    return FColor(130, 150, 175);
		case EScreenLayer::Menu:   return FColor(110, 190, 250);
		case EScreenLayer::Modal:  return FColor(250, 180, 90);
		case EScreenLayer::Popup:  return FColor(160, 230, 150);
		case EScreenLayer::System: return FColor(240, 120, 120);
		default:                   return FColor::White;
		}
	}

	static const TCHAR* LayerName(EScreenLayer L)
	{
		switch (L)
		{
		case EScreenLayer::HUD:    return TEXT("HUD");
		case EScreenLayer::Menu:   return TEXT("Menu");
		case EScreenLayer::Modal:  return TEXT("Modal");
		case EScreenLayer::Popup:  return TEXT("Popup");
		case EScreenLayer::System: return TEXT("System");
		default:                   return TEXT("?");
		}
	}

	static const TCHAR* ModeName(EScreenInputMode M)
	{
		switch (M)
		{
		case EScreenInputMode::Game:      return TEXT("Game");
		case EScreenInputMode::GameAndUI: return TEXT("GameAndUI");
		case EScreenInputMode::UI:        return TEXT("UI");
		default:                          return TEXT("Inherit");
		}
	}

	static FScreenEntry Mach(const TCHAR* Id, EScreenLayer L, EScreenInputMode Input,
		EScreenOpinion Cursor, EScreenOpinion Pause, bool bBlocks = false, bool bHides = false)
	{
		FScreenEntry E;
		E.Id = Id;
		E.Layer = L;
		E.InputMode = Input;
		E.Cursor = Cursor;
		E.Pause = Pause;
		E.bBlocksBelow = bBlocks;
		E.bHidesBelow = bHides;
		return E;
	}
}

AScreenStackDemoDirector::AScreenStackDemoDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BoardText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BoardText"));
	BoardText->SetupAttachment(Root);
	BoardText->SetHorizontalAlignment(EHTA_Center);
	BoardText->SetVerticalAlignment(EVRTA_TextBottom);
	BoardText->SetWorldSize(40.0f);
	BoardText->SetTextRenderColor(FColor::White);

	// Yaw 270, not 90: at 90 a TextRender renders mirrored.
	BoardText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
	BoardText->SetRelativeLocation(FVector(0.0f, 0.0f, 760.0f));

	// Offsets on X, not Y - the camera looks along +Y, so Y is depth.
	StackText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StackText"));
	StackText->SetupAttachment(Root);
	StackText->SetHorizontalAlignment(EHTA_Center);
	StackText->SetVerticalAlignment(EVRTA_TextTop);
	StackText->SetWorldSize(28.0f);
	StackText->SetTextRenderColor(FColor(205, 215, 230));
	StackText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
	StackText->SetRelativeLocation(FVector(-640.0f, 0.0f, 620.0f));

	StateText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StateText"));
	StateText->SetupAttachment(Root);
	StateText->SetHorizontalAlignment(EHTA_Center);
	StateText->SetVerticalAlignment(EVRTA_TextTop);
	StateText->SetWorldSize(32.0f);
	StateText->SetTextRenderColor(FColor(255, 200, 110));
	StateText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
	StateText->SetRelativeLocation(FVector(680.0f, 0.0f, 620.0f));
}

void AScreenStackDemoDirector::BeginPlay()
{
	Super::BeginPlay();
	StartCycle();
}

void AScreenStackDemoDirector::StartCycle()
{
	using namespace ScreenStackDemoLocal;

	CycleTime = 0.0f;
	Naechster = 0;
	Letzte.Reset();

	if (UScreenStackSubsystem* S = UScreenStackStatics::GetScreenStack(this))
	{
		S->Clear();
	}

	Skript.Reset();
	const auto Push = [this](float Wann, const FScreenEntry& E, const TCHAR* Text)
	{
		FSchritt Sch;
		Sch.Wann = Wann;
		Sch.bPush = true;
		Sch.Eintrag = E;
		Sch.Text = Text;
		Skript.Add(Sch);
	};
	const auto Pop = [this](float Wann, const TCHAR* Id, const TCHAR* Text)
	{
		FSchritt Sch;
		Sch.Wann = Wann;
		Sch.bPush = false;
		Sch.Id = Id;
		Sch.Text = Text;
		Skript.Add(Sch);
	};

	Push(1.0f, Mach(TEXT("HUD"), EScreenLayer::HUD, EScreenInputMode::Inherit,
		EScreenOpinion::Inherit, EScreenOpinion::Inherit), TEXT("push HUD - it has no opinion about anything"));
	Push(4.0f, Mach(TEXT("Inventory"), EScreenLayer::Menu, EScreenInputMode::UI,
		EScreenOpinion::Yes, EScreenOpinion::Yes, false, true), TEXT("push the inventory - UI, cursor, and it pauses"));
	Push(8.0f, Mach(TEXT("Confirm"), EScreenLayer::Modal, EScreenInputMode::UI,
		EScreenOpinion::Yes, EScreenOpinion::Inherit, true), TEXT("push a confirmation - it blocks what is under it"));

	// The first beat.
	Push(12.0f, Mach(TEXT("Compass"), EScreenLayer::HUD, EScreenInputMode::Game,
		EScreenOpinion::No, EScreenOpinion::Inherit),
		TEXT("push a HUD element - it lands UNDER the modal and steals nothing"));

	Pop(16.0f, TEXT("Confirm"), TEXT("pop the confirmation - the inventory gets the input back"));
	Push(19.0f, Mach(TEXT("Toast"), EScreenLayer::Popup, EScreenInputMode::Inherit,
		EScreenOpinion::Inherit, EScreenOpinion::Inherit), TEXT("a toast pops up - no opinion about anything"));

	// The second beat.
	Pop(23.0f, TEXT("Toast"), TEXT("pop the toast - the game is STILL paused, the menu holds it"));
	Pop(26.0f, nullptr, TEXT("clear - input back to the game, cursor off, unpaused"));

	bBereit = true;
}

void AScreenStackDemoDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// The editor hands out one enormous delta after a recompile.
	DeltaSeconds = FMath::Clamp(DeltaSeconds, 0.0f, 0.1f);

	if (!bBereit)
	{
		StartCycle();
	}

	CycleTime += DeltaSeconds;
	if (CycleTime > CycleSeconds)
	{
		StartCycle();
		return;
	}

	UScreenStackSubsystem* S = UScreenStackStatics::GetScreenStack(this);
	if (!S)
	{
		return;
	}

	while (Naechster < Skript.Num() && CycleTime >= Skript[Naechster].Wann)
	{
		const FSchritt& Sch = Skript[Naechster++];
		if (Sch.bPush)
		{
			S->Push(Sch.Eintrag);
		}
		else if (Sch.Id.IsNone())
		{
			S->Clear();
		}
		else
		{
			S->Pop(Sch.Id);
		}
		Letzte = Sch.Text;
	}

	if (BoardText)
	{
		BoardText->SetText(FText::FromString(Letzte.IsEmpty()
			? TEXT("an empty stack: the game owns the input") : Letzte));
	}
	if (StackText)
	{
		StackText->SetText(FText::FromString(BuildStack()));
	}
	if (StateText)
	{
		StateText->SetText(FText::FromString(BuildState()));
	}

	if (bDrawDemo)
	{
		DrawStack();
	}
}

FString AScreenStackDemoDirector::BuildStack() const
{
	const UScreenStackSubsystem* S = UScreenStackStatics::GetScreenStack(this);
	if (!S)
	{
		return FString();
	}
	const TArray<FScreenEntry>& Stapel = S->GetStack();

	FString T = TEXT("THE STACK  (top first)\n");
	if (Stapel.Num() == 0)
	{
		T += TEXT("  -- empty --\n");
		return T;
	}

	for (int32 i = Stapel.Num() - 1; i >= 0; --i)
	{
		const FScreenEntry& E = Stapel[i];
		T += FString::Printf(TEXT("%-7s %-10s %-9s%s%s%s\n"),
			ScreenStackDemoLocal::LayerName(E.Layer), *E.Id.ToString(),
			ScreenStackDemoLocal::ModeName(E.InputMode),
			E.Pause == EScreenOpinion::Yes ? TEXT(" pause") : TEXT(""),
			UScreenStackStatics::IsVisible(i, Stapel) ? TEXT("") : TEXT(" hidden"),
			UScreenStackStatics::IsInteractive(i, Stapel) ? TEXT("") : TEXT(" blocked"));
	}
	return T;
}

FString AScreenStackDemoDirector::BuildState() const
{
	const UScreenStackSubsystem* S = UScreenStackStatics::GetScreenStack(this);
	if (!S)
	{
		return FString();
	}
	const FScreenState Z = S->GetState();

	return FString::Printf(
		TEXT("RESOLVES TO\n\ninput   %s\ncursor  %s\npaused  %s\n\ntop     %s\ndrawn   %d of %d"),
		ScreenStackDemoLocal::ModeName(Z.InputMode),
		Z.bCursor ? TEXT("on") : TEXT("off"),
		Z.bPaused ? TEXT("YES") : TEXT("no"),
		Z.TopInteractiveId.IsNone() ? TEXT("-") : *Z.TopInteractiveId.ToString(),
		Z.VisibleCount, Z.Depth);
}

void AScreenStackDemoDirector::DrawStack() const
{
#if ENABLE_DRAW_DEBUG
	const UWorld* World = GetWorld();
	const UScreenStackSubsystem* S = UScreenStackStatics::GetScreenStack(this);
	if (!World || !S)
	{
		return;
	}
	UWorld* Mutable = const_cast<UWorld*>(World);

	// 0.35 s, not one frame: during a screenshot run the editor renders many frames between two
	// world ticks, and anything shorter is gone by the time the picture is taken.
	constexpr float Life = 0.35f;
	constexpr float Breite = 520.0f;
	constexpr float Schritt = 70.0f;

	const TArray<FScreenEntry>& Stapel = S->GetStack();
	const FVector Fuss = GetActorLocation() + FVector(260.0f, 0.0f, 170.0f);

	for (int32 i = 0; i < Stapel.Num(); ++i)
	{
		const FScreenEntry& E = Stapel[i];

		// Drawn bottom up, so the picture matches the word "stack" - index 0 at the bottom.
		const FVector Zeile = Fuss + FVector(0.0f, 0.0f, i * Schritt);
		const FColor Farbe = ScreenStackDemoLocal::LayerColour(E.Layer);

		// Each layer is indented by its own order, so the HUD that landed under the modal is
		// visibly under it rather than merely earlier in a list.
		const float Einzug = static_cast<float>(E.Layer) * 42.0f;

		for (int32 Ply = 0; Ply < 4; ++Ply)
		{
			const FVector Hoch(0.0f, 0.0f, Ply * 9.0f);
			DrawDebugLine(Mutable, Zeile + Hoch + FVector(Einzug, 0.0f, 0.0f),
				Zeile + Hoch + FVector(Einzug - Breite, 0.0f, 0.0f),
				UScreenStackStatics::IsInteractive(i, Stapel) ? Farbe
					: FColor(Farbe.R / 3, Farbe.G / 3, Farbe.B / 3),
				false, Life, 0, 10.0f);
		}

		// A layer that is not drawn gets a line through it - hidden and blocked are different
		// things, and the picture has to show both.
		if (!UScreenStackStatics::IsVisible(i, Stapel))
		{
			DrawDebugLine(Mutable, Zeile + FVector(Einzug, 0.0f, 18.0f),
				Zeile + FVector(Einzug - Breite, 0.0f, 18.0f),
				FColor(90, 92, 104), false, Life, 0, 3.0f);
		}
	}
#endif
}
