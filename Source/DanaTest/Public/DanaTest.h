// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "CopyFoliage.h"

class FToolBarBuilder;
class FMenuBuilder;

class FDanaTestModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void OnCopyClick();
	void OnGenerateClick();
	void OnSnapClick();
	
private:

	void RegisterMenus();

private:
	TSharedPtr<class FUICommandList> PluginCommands;
	TSharedPtr<CopyFoliage> CopyFoliageTool;
	
private: 
	void AddToolbarExtension(FToolBarBuilder& Builder);
	TSharedRef<SWidget> GetComboConent();
};
