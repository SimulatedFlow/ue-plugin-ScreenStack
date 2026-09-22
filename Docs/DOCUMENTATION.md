# ScreenStack — UI Layers, Modals & Input Ownership

**Who owns the input right now?**

Every project answers that with a pile of booleans spread across widgets, and every project ships
the same three bugs.

---

## 0. Supported engine and platforms

* Unreal Engine **5.8**
* **Win64**. One runtime C++ module, no third-party code, full source included.
* No UMG dependency. ScreenStack ships **no widgets and no menus** — a project that draws its UI some
  other way must not have to take UMG for this.

---

## 1. The five-minute install

Wherever you open a screen:

```
Push (Id="Inventory", Layer=Menu, InputMode=UI, Cursor=Yes, Pause=Yes, bHidesBelow=true)
```

and where you close it:

```
Pop ("Inventory")
```

That is the whole API. The resolved input mode, cursor and pause are applied to the first local
player controller after every change, and only when they actually changed.

Bind `OnScreenStateChanged` if your project wants to react to it — an audio mix snapshot, a blur, a
gamepad focus pass.

---

## 2. Layers are ordered

`HUD < Menu < Modal < Popup < System`

A push lands **by layer**, not on top. `InsertIndexFor` puts it after everything of the same layer or
lower, before anything higher.

Without that, the damage indicator your HUD pushed while a confirmation dialog was open takes the
input away from the dialog — and the player answers the dialog by accident with a movement key.

Within one layer, a push goes above what is already there, so two popups keep the order they were
opened in.

---

## 3. Everything is resolved, never remembered

This is the difference that fixes the bugs.

| Rule | How it resolves |
|---|---|
| **Input mode** | The topmost layer with an opinion. `Inherit` layers are transparent. Empty stack → `Game`. |
| **Cursor** | The topmost layer with an opinion. None → off. |
| **Pause** | **An OR across the whole stack.** |

### Why pause is an OR

A popup opens over the pause menu. The popup has no opinion about pausing. The popup closes. An
implementation that restores "what the top layer wants" **un-pauses a game the menu underneath is
still holding**.

An OR cannot do that. While any layer on the stack wants the pause, the game stays paused.

Note what that means for `No`: for pausing, it is the same as `Inherit`. A layer cannot take a pause
away from a layer holding one, because *"I do not need the pause"* and *"nobody may have the pause"*
are different statements and only the first is safe to honour. A layer that really must have the
game running should pop the one holding it. `No` still means No for the cursor and the input mode,
where the topmost opinion wins outright.

### Why resolve rather than remember

A widget that saves the state it found and restores it on close is correct exactly until two of them
are open at once. Then the inner one restores what it saw — which was the outer one's state — and the
outer one restores what *it* saw, which was the game's. The stack cannot get out of step with itself
the way two remembered copies can.

---

## 4. Hidden is not the same as blocked

`bHidesBelow` — nothing below is drawn. `bBlocksBelow` — nothing below can be interacted with.

They are separate because a dimming modal lets you **see** the inventory behind it and must not let
you **click** it. `IsVisible(Index, Stack)` and `IsInteractive(Index, Stack)` answer the two
questions separately, and the demo shows one layer that is each.

---

## 5. Ids are unique

Pushing an id that is already on the stack **replaces its entry in place**. Adding a second would
leave a layer nobody can pop — `Pop` takes the first match and the other stays forever.

`Pop` on something that is not there, and `PopTop` on an empty stack, are no-ops rather than
warnings. Closing a screen that is already closed happens on every double click of a back button.

---

## 6. What ScreenStack is not

* **It ships no menus and no widgets.** It is the plumbing under whatever you already have.
* **It does not do gamepad focus navigation.** That is a different, well-covered problem — pair it
  with the navigation plugin of your choice. ScreenStack decides which layer owns the input; what
  happens to focus inside that layer is not its business.
* **It does not create, destroy or draw anything.** Push and pop are bookkeeping; adding the widget
  to the viewport stays yours.
* **It is not replicated.** UI state is local by definition.
* **Pause goes through the player controller**, not the world, so a game mode that refuses pausing
  keeps refusing it. The plugin states what the UI wants; it does not overrule the rules of the game.

---

## 7. Console commands

| Command | What it does |
|---|---|
| `ScreenStack.Dump` | The stack top down, with the resolved input mode, cursor and pause. |
| `ScreenStack.Pop` | Pop the top layer. The way out when a screen has trapped the input. |
| `ScreenStack.Clear` | Empty the stack and hand the input back to the game. |

Those last two have saved more time than anything else in the plugin: a UI bug that locks the player
out of the game no longer needs a restart to investigate.

---

## 8. API reference

### `UScreenStackSubsystem`

`Push(FScreenEntry)`, `Pop(FName)`, `PopTop()`, `Clear()`, `Contains(FName)`, `GetState()`,
`GetStack()`, `ApplyTo(APlayerController*)`.
Delegate: `OnScreenStateChanged(const FScreenState&)`.

`ApplyTo` is public because a split-screen project has more than one local player, and a project
with its own input plumbing may want to apply the resolved state somewhere else entirely.

### `UScreenStackStatics` — the rules, on their own

`ResolveState`, `ResolveInputMode`, `ResolveCursor`, `ResolvePause`, `InsertIndexFor`, `IsVisible`,
`IsInteractive`.

Every one takes the **whole stack**. That is the fix, not an implementation detail.

---

## 9. The demo level

`Content/ScreenStack/Maps/L_ScreenStackDemo` runs **without pressing play**. A stack being built and
taken apart, with the resolved state beside it, built around the two beats: a HUD element pushed
while a modal is open lands under it, and a popup closing over a pause menu leaves the game paused.

---

## 10. Troubleshooting

**The player is stuck in UI mode.** Something was pushed and never popped. `ScreenStack.Dump` names
it; `ScreenStack.Clear` gets you out.

**The game will not un-pause.** Some layer still on the stack has `Pause = Yes`. That is the rule
working — find it in the dump.

**A menu opens and the input does not change.** Its `InputMode` is `Inherit`. A layer with no opinion
is transparent on purpose.

**A pushed layer does not appear on top.** It is not supposed to if something of a higher layer is
open. Check the `Layer` you gave it.

**Pausing does nothing.** Your game mode has `bPauseable` off, or there is no player controller yet.
ScreenStack asks; the game decides.
