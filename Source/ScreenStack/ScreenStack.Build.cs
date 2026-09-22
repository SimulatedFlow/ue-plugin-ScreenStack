// Copyright 2026 Silvan Teufel. All Rights Reserved.

using UnrealBuildTool;

public class ScreenStack : ModuleRules
{
	public ScreenStack(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",

			// APlayerController and the input modes; UGameplayStatics for the first local player.
			"Engine",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			// FInputModeUIOnly and friends live in the Slate headers the Engine module pulls in;
			// nothing else is needed. In particular NOT UMG: ScreenStack ships no widgets, and a
			// project that draws its UI some other way must not have to take UMG for this.
		});
	}
}
