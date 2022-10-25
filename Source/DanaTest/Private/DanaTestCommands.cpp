// Copyright Epic Games, Inc. All Rights Reserved.

#include "DanaTestCommands.h"

#define LOCTEXT_NAMESPACE "FDanaTestModule"

void FDanaTestCommands::RegisterCommands()
{
	//UI_COMMAND(OpenPluginWindow, "DanaTest", "Bring up DanaTest window", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(OpenCopyWindow, "Copy", "", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(OpenGenerateWindow, "Generate", "", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(SnapFoliage, "Snap", "", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
