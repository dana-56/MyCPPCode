// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "DanaTestStyle.h"

class FDanaTestCommands : public TCommands<FDanaTestCommands>
{
public:

	FDanaTestCommands()
		: TCommands<FDanaTestCommands>(TEXT("DanaTest"), NSLOCTEXT("Contexts", "DanaTest", "DanaTest Plugin"), NAME_None, FDanaTestStyle::GetStyleSetName())
	{
	}

	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenCopyWindow;
	TSharedPtr< FUICommandInfo > OpenGenerateWindow;
	TSharedPtr< FUICommandInfo > SnapFoliage;
};