// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NeoSanctum : ModuleRules
{
	public NeoSanctum(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[] { "ProceduralDungeon" });
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			// Core
			"Core", "CoreUObject", "Engine", "OnlineSubsystem", "OnlineSubsystemUtils", "NetCore",

			// Settings
			"DeveloperSettings",
			
			// Input
			"InputCore", "EnhancedInput",
			
			// UI
			"UMG", 
			"Slate", 
			"SlateCore", 
			"CommonUI", 
			"CommonInput",
			"GameplayMessageRuntime",
			
			// GAS
			"GameplayAbilities", 
			"GameplayTags", 
			"GameplayTasks",
			
			// AI
			"AIModule", 
			"GameplayStateTreeModule", 
			"NavigationSystem",
			"StateTreeModule",
			
			// Animation
			"AnimGraphRuntime", 
			"MotionTrajectory",
			"PoseSearch",
			"Chooser",
			"MotionWarping",
			
			// FX
			"Niagara",
			
			// World
			"PCG",
			
			// Physics / Trace
			"PhysicsCore",
			
			// Chaos Destruction
			"GeometryCollectionEngine",
			"Chaos",
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
