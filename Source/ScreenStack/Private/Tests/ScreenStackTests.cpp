// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "ScreenStackStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ScreenStackTests
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::CommandletContext
		| EAutomationTestFlags::EngineFilter;

	static FScreenEntry Ebene(const TCHAR* Id, EScreenLayer Layer,
		EScreenInputMode Input = EScreenInputMode::Inherit,
		EScreenOpinion Cursor = EScreenOpinion::Inherit,
		EScreenOpinion Pause = EScreenOpinion::Inherit)
	{
		FScreenEntry E;
		E.Id = Id;
		E.Layer = Layer;
		E.InputMode = Input;
		E.Cursor = Cursor;
		E.Pause = Pause;
		return E;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FScreenStackPopRestoresFromTheStack,
	"ScreenStack.State.PopRestoresFromTheStackNotFromMemory", ScreenStackTests::TestFlags)

bool FScreenStackPopRestoresFromTheStack::RunTest(const FString&)
{
	using namespace ScreenStackTests;

	TArray<FScreenEntry> Stapel;
	Stapel.Add(Ebene(TEXT("HUD"), EScreenLayer::HUD));  // no opinion about anything
	TestEqual(TEXT("a HUD alone leaves the game in charge"),
		UScreenStackStatics::ResolveInputMode(Stapel), EScreenInputMode::Game);

	Stapel.Add(Ebene(TEXT("Inventory"), EScreenLayer::Menu, EScreenInputMode::GameAndUI,
		EScreenOpinion::Yes, EScreenOpinion::Yes));
	TestEqual(TEXT("the menu takes it"),
		UScreenStackStatics::ResolveInputMode(Stapel), EScreenInputMode::GameAndUI);

	Stapel.Add(Ebene(TEXT("Confirm"), EScreenLayer::Modal, EScreenInputMode::UI));
	TestEqual(TEXT("the modal takes it from the menu"),
		UScreenStackStatics::ResolveInputMode(Stapel), EScreenInputMode::UI);

	// THE RULE. Popping the modal restores what the MENU asked for, worked out from the stack. An
	// implementation where each widget remembers the state it found gets this wrong the moment two
	// of them are open at once.
	Stapel.Pop();
	TestEqual(TEXT("popping the modal gives it back to the menu"),
		UScreenStackStatics::ResolveInputMode(Stapel), EScreenInputMode::GameAndUI);

	Stapel.Pop();
	TestEqual(TEXT("and popping the menu gives it back to the game"),
		UScreenStackStatics::ResolveInputMode(Stapel), EScreenInputMode::Game);

	// An empty stack is the game, not "whatever it was last".
	Stapel.Reset();
	const FScreenState Leer = UScreenStackStatics::ResolveState(Stapel);
	TestEqual(TEXT("empty means Game"), Leer.InputMode, EScreenInputMode::Game);
	TestFalse(TEXT("empty means no cursor"), Leer.bCursor);
	TestFalse(TEXT("empty means not paused"), Leer.bPaused);
	TestEqual(TEXT("and nothing on top"), Leer.TopInteractiveId, FName(NAME_None));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FScreenStackPauseIsAnOr,
	"ScreenStack.State.PauseIsAnOrAcrossTheWholeStack", ScreenStackTests::TestFlags)

bool FScreenStackPauseIsAnOr::RunTest(const FString&)
{
	using namespace ScreenStackTests;

	TArray<FScreenEntry> Stapel;
	Stapel.Add(Ebene(TEXT("PauseMenu"), EScreenLayer::Menu, EScreenInputMode::UI,
		EScreenOpinion::Yes, EScreenOpinion::Yes));
	TestTrue(TEXT("the pause menu pauses"), UScreenStackStatics::ResolvePause(Stapel));

	// A popup with no opinion opens over it.
	Stapel.Add(Ebene(TEXT("Toast"), EScreenLayer::Popup));
	TestTrue(TEXT("a popup with no opinion does not un-pause it"),
		UScreenStackStatics::ResolvePause(Stapel));

	// THE BUG THIS FIXES. An implementation that restores "what the top layer wants" un-pauses the
	// game here, while the pause menu is still open behind the popup.
	Stapel.Pop();
	TestTrue(TEXT("and closing the popup leaves it paused"), UScreenStackStatics::ResolvePause(Stapel));

	// A layer saying No cannot take a pause away from a layer holding one - "I do not need the
	// pause" and "nobody may have it" are different statements.
	Stapel.Add(Ebene(TEXT("Spectate"), EScreenLayer::Popup, EScreenInputMode::Inherit,
		EScreenOpinion::Inherit, EScreenOpinion::No));
	TestTrue(TEXT("No does not overrule a Yes underneath"), UScreenStackStatics::ResolvePause(Stapel));

	// With the holder gone, nothing is holding it.
	Stapel.RemoveAll([](const FScreenEntry& E) { return E.Id == FName(TEXT("PauseMenu")); });
	TestFalse(TEXT("nobody wants it, nobody gets it"), UScreenStackStatics::ResolvePause(Stapel));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FScreenStackPushLandsByLayer,
	"ScreenStack.Order.APushLandsByLayerNotOnTop", ScreenStackTests::TestFlags)

bool FScreenStackPushLandsByLayer::RunTest(const FString&)
{
	using namespace ScreenStackTests;

	TArray<FScreenEntry> Stapel;
	Stapel.Add(Ebene(TEXT("HUD"), EScreenLayer::HUD));
	Stapel.Add(Ebene(TEXT("Confirm"), EScreenLayer::Modal, EScreenInputMode::UI));

	// A second HUD element is pushed while the modal is open - a damage indicator, a quest toast.
	const int32 Wohin = UScreenStackStatics::InsertIndexFor(EScreenLayer::HUD, Stapel);
	TestEqual(TEXT("it lands under the modal"), Wohin, 1);

	Stapel.Insert(Ebene(TEXT("Compass"), EScreenLayer::HUD, EScreenInputMode::Game), Wohin);

	// THE POINT. Landing on top would hand the input back to the game while a confirmation dialog
	// is open - and the player would answer it by accident with a movement key.
	TestEqual(TEXT("the modal still owns the input"),
		UScreenStackStatics::ResolveInputMode(Stapel), EScreenInputMode::UI);
	TestEqual(TEXT("and the modal is still on top"),
		UScreenStackStatics::ResolveState(Stapel).TopInteractiveId, FName(TEXT("Confirm")));

	// Layer order holds all the way up.
	TestEqual(TEXT("a system layer goes above everything"),
		UScreenStackStatics::InsertIndexFor(EScreenLayer::System, Stapel), Stapel.Num());
	TestEqual(TEXT("and a menu between HUD and modal"),
		UScreenStackStatics::InsertIndexFor(EScreenLayer::Menu, Stapel), 2);

	// Same layer: after the ones already there, so two popups keep the order they were opened in.
	Stapel.Reset();
	Stapel.Add(Ebene(TEXT("A"), EScreenLayer::Popup));
	TestEqual(TEXT("a second popup goes above the first"),
		UScreenStackStatics::InsertIndexFor(EScreenLayer::Popup, Stapel), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FScreenStackCursorIsTheTopOpinion,
	"ScreenStack.State.CursorIsTheTopmostOpinion", ScreenStackTests::TestFlags)

bool FScreenStackCursorIsTheTopOpinion::RunTest(const FString&)
{
	using namespace ScreenStackTests;

	TArray<FScreenEntry> Stapel;
	Stapel.Add(Ebene(TEXT("Menu"), EScreenLayer::Menu, EScreenInputMode::UI, EScreenOpinion::Yes));
	TestTrue(TEXT("the menu shows the cursor"), UScreenStackStatics::ResolveCursor(Stapel));

	// A HUD layer with no opinion is transparent - it must not take the cursor away.
	Stapel.Insert(Ebene(TEXT("HUD"), EScreenLayer::HUD), 0);
	TestTrue(TEXT("a layer with no opinion changes nothing"),
		UScreenStackStatics::ResolveCursor(Stapel));

	// A layer that actively says No, above it, does.
	Stapel.Add(Ebene(TEXT("Cinematic"), EScreenLayer::System, EScreenInputMode::Inherit,
		EScreenOpinion::No));
	TestFalse(TEXT("an explicit No above wins"), UScreenStackStatics::ResolveCursor(Stapel));

	Stapel.Pop();
	TestTrue(TEXT("and popping it gives the cursor back"), UScreenStackStatics::ResolveCursor(Stapel));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FScreenStackVisibleAndInteractiveDiffer,
	"ScreenStack.Order.VisibleAndInteractiveAreDifferentQuestions", ScreenStackTests::TestFlags)

bool FScreenStackVisibleAndInteractiveDiffer::RunTest(const FString&)
{
	using namespace ScreenStackTests;

	TArray<FScreenEntry> Stapel;
	Stapel.Add(Ebene(TEXT("HUD"), EScreenLayer::HUD));
	Stapel.Add(Ebene(TEXT("Inventory"), EScreenLayer::Menu, EScreenInputMode::UI));

	// A dimming modal: you can still SEE the inventory behind it, and must not be able to click it.
	FScreenEntry Modal = Ebene(TEXT("Confirm"), EScreenLayer::Modal, EScreenInputMode::UI);
	Modal.bBlocksBelow = true;
	Stapel.Add(Modal);

	TestTrue(TEXT("the inventory is still drawn"), UScreenStackStatics::IsVisible(1, Stapel));
	TestFalse(TEXT("but cannot be clicked"), UScreenStackStatics::IsInteractive(1, Stapel));
	TestTrue(TEXT("the modal itself is both"),
		UScreenStackStatics::IsVisible(2, Stapel) && UScreenStackStatics::IsInteractive(2, Stapel));

	// A full-screen menu that hides what is below.
	Stapel[1].bHidesBelow = true;
	TestFalse(TEXT("the HUD is not drawn under a hiding menu"),
		UScreenStackStatics::IsVisible(0, Stapel));

	const FScreenState S = UScreenStackStatics::ResolveState(Stapel);
	TestEqual(TEXT("two of the three are drawn"), S.VisibleCount, 2);
	TestEqual(TEXT("and the depth is still three"), S.Depth, 3);

	// Out of range is false rather than undefined.
	TestFalse(TEXT("a bad index is not visible"), UScreenStackStatics::IsVisible(99, Stapel));
	TestFalse(TEXT("nor interactive"), UScreenStackStatics::IsInteractive(-1, Stapel));

	return true;
}

#endif  // WITH_DEV_AUTOMATION_TESTS
