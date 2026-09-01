// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class DirtbagUE : ModuleRules
{
	public DirtbagUE(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"DirtbagUE",
			"DirtbagUE/Variant_Platforming",
			"DirtbagUE/Variant_Platforming/Animation",
			"DirtbagUE/Variant_Combat",
			"DirtbagUE/Variant_Combat/AI",
			"DirtbagUE/Variant_Combat/Animation",
			"DirtbagUE/Variant_Combat/Gameplay",
			"DirtbagUE/Variant_Combat/Interfaces",
			"DirtbagUE/Variant_Combat/UI",
			"DirtbagUE/Variant_SideScrolling",
			"DirtbagUE/Variant_SideScrolling/AI",
			"DirtbagUE/Variant_SideScrolling/Gameplay",
			"DirtbagUE/Variant_SideScrolling/Interfaces",
			"DirtbagUE/Variant_SideScrolling/UI"
		});

		// The engine-free sim core lives at the repo root (../../../Sim from
		// this module) so the same translation units also compile in the
		// standalone g++ harness (Sim/run-tests.sh). The Sim*.cpp bridge
		// files in this module pull its .cpp files into the build; this
		// include path lets everything say #include "DirtbagSession.h".
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "../../../Sim"));

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
