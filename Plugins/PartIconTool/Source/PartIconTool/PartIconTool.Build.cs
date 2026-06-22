using UnrealBuildTool;

public class PartIconTool : ModuleRules
{
	public PartIconTool(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"DeveloperSettings",
				"Engine",
				"RenderCore",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"ToolMenus",
				"ContentBrowser",
				"AssetRegistry",
				// 파츠 Definition 타입 접근용 (에디터에서만 로드되는 게임 모듈)
				"NeoSanctum",
			}
		);
	}
}
