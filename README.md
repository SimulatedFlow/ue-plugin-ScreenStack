# ScreenStack — UI Layers, Modals & Input Ownership

**Unreal Engine 5.8 · Win64 · one runtime C++ module · Blueprint-first · full source**

Who owns the input right now? Every project answers that with a pile of booleans spread across
widgets, and every project ships the same bugs: a popup closes and un-pauses a game the menu
underneath is still holding; a HUD element is pushed while a modal is open and quietly steals the
gamepad; a modal over a modal closes and restores the wrong state because each widget remembered
what it thought the state was instead of asking.

ScreenStack is the plumbing under your menus. Layers are ordered, so a push lands where it belongs.
Input mode and cursor are the topmost **opinion**, and layers with none are transparent. Pause is an
**OR** across the whole stack — the single rule that fixes the popup bug. Everything is computed from
the stack on every change, never remembered.

It ships **no menus and no widgets**, and deliberately does not do gamepad focus navigation: it is
the layer under whatever you already use for that.

## Install in five minutes

1. **Push (Id, Layer, InputMode, Cursor, Pause)** where you open a screen.
2. **Pop (Id)** where you close it.

That is the whole API.

## What is in the box

* `UScreenStackSubsystem` — the stack, and the one place that applies what it resolves to
* `UScreenStackStatics` — the rules as pure functions, each taking the whole stack
* Console commands `ScreenStack.Dump`, `.Pop` and `.Clear` — the way out of a UI that has
  trapped the input, without a restart
* A demo level that runs **without pressing play**
* Five automation tests over the rules that fail quietly

## Documentation

https://wiki.teufel-engineering.com/en/ScreenStack/documentation

## Support

teufelsilvan@gmail.com

Copyright 2026 Silvan Teufel. All Rights Reserved.

<!-- SF-STORE-BLOCK:BEGIN -->
## 🛒 Source-available — see before you buy

This repository contains the **full source** of a commercial Unreal Engine plugin. It is **source-available, not open source**: read it, evaluate it, then buy a license to use it. See **the Fab Content License Agreement / Unreal Engine EULA (purchase required)**.

**Get it / Buy:**
- Fab store — all our UE5 plugins: https://www.fab.com/sellers/Silvan%20Teufel

_This plugin does not have its own Fab listing yet — the store link above is where everything we currently sell lives._

### 📬 **Free UE5 Snippet-Pack**

10 ready-to-use C++/Blueprint building blocks (subsystems, versioned saves, async nodes, editor tooling) — MIT licensed. Get it by joining the newsletter — plus a heads-up when something new ships. Double opt-in, unsubscribe in one click, no address sharing.

👉 **[Get the free pack](https://silvan.teufel-engineering.com/newsletter/plugins/?q=gh)**

_© 2026 Silvan Teufel. All rights reserved._
<!-- SF-STORE-BLOCK:END -->
