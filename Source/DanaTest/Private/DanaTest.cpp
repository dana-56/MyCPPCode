// Copyright Epic Games, Inc. All Rights Reserved.

#include "DanaTest.h"
#include "DanaTestStyle.h"
#include "DanaTestCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FDanaTestModule"

void FDanaTestModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FDanaTestStyle::Initialize();
	FDanaTestStyle::ReloadTextures();

	FDanaTestCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FDanaTestCommands::Get().OpenCopyWindow,
		FExecuteAction::CreateRaw(this, &FDanaTestModule::OnCopyClick),
		FCanExecuteAction());

	PluginCommands->MapAction(
		FDanaTestCommands::Get().OpenGenerateWindow,
		FExecuteAction::CreateRaw(this, &FDanaTestModule::OnGenerateClick),
		FCanExecuteAction());

	PluginCommands->MapAction(
		FDanaTestCommands::Get().SnapFoliage,
		FExecuteAction::CreateRaw(this, &FDanaTestModule::OnSnapClick),
		FCanExecuteAction());


	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After,
			PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FDanaTestModule::AddToolbarExtension));
		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}

	CopyFoliageTool = MakeShareable(new CopyFoliage);
}

void FDanaTestModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FDanaTestStyle::Shutdown();

	FDanaTestCommands::Unregister();
}

void FDanaTestModule::OnCopyClick()
{
	CopyFoliageTool->Copy();
}

void FDanaTestModule::OnGenerateClick()
{
	CopyFoliageTool->Generate();
}

void FDanaTestModule::OnSnapClick()
{
	CopyFoliageTool->Snap();
}

void FDanaTestModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddComboButton(FUIAction(), FOnGetContent::CreateRaw(this, &FDanaTestModule::GetComboConent),
		FText::FromString("FoliageTool"), FText::FromString("FoliageTool"),
		FSlateIcon("DanaTestStyle", "DanaTest.Root"));
}

TSharedRef<SWidget> FDanaTestModule::GetComboConent()
{
	FMenuBuilder MenuBuilder(true, PluginCommands);

	//Foliage Tool
	struct FLocalMenuBuilder
	{
		static void FillViewportSyncMenu(FMenuBuilder& InSubMenuBuilder)
		{
			InSubMenuBuilder.AddMenuEntry(FDanaTestCommands::Get().OpenCopyWindow);
			InSubMenuBuilder.AddMenuEntry(FDanaTestCommands::Get().OpenGenerateWindow);
			InSubMenuBuilder.AddMenuEntry(FDanaTestCommands::Get().SnapFoliage);
		}
	};

	MenuBuilder.AddSubMenu(
		LOCTEXT("FoliageViewport", "Foliage"),
		LOCTEXT("FoliageViewport_ToolTip", "Foliage"),
		FNewMenuDelegate::CreateStatic(&FLocalMenuBuilder::FillViewportSyncMenu),
		false,
		FSlateIcon(FDanaTestStyle::GetStyleSetName(),"DanaTest.FoliageTool"));
	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FDanaTestModule, DanaTest)