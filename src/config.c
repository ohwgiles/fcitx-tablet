/**************************************************************************
 *
 *  fcitx-tablet : graphics tablet input for fcitx input method framework
 *  Copyright 2012  Oliver Giles
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 **************************************************************************/

#include <fcitx/instance.h>
#include <fcitx-utils/log.h>
#include <fcitx-config/xdg.h>
#include <errno.h>
#include "config.h"

// config.c
// This file handles the loading and saving of the config file

CONFIG_DESC_DEFINE(GetFcitxTabletConfigDesc, "fcitx-tablet.desc")
CONFIG_BINDING_BEGIN(FcitxTabletConfig);
CONFIG_BINDING_REGISTER("Tablet", "Driver", Driver);
CONFIG_BINDING_REGISTER("Tablet", "Engine", Engine);
CONFIG_BINDING_REGISTER("Tablet", "ZinniaModel", ZinniaModel);
CONFIG_BINDING_REGISTER("Tablet", "ForkEngine", ForkEngine);
CONFIG_BINDING_REGISTER("Tablet", "XPos", XPos);
CONFIG_BINDING_REGISTER("Tablet", "YPos", YPos);
CONFIG_BINDING_REGISTER("Tablet", "Width", Width);
CONFIG_BINDING_REGISTER("Tablet", "Height", Height);
CONFIG_BINDING_REGISTER("Tablet", "BorderWidth", BorderWidth);
CONFIG_BINDING_REGISTER("Tablet", "BackgroundColour", BackgroundColour);
CONFIG_BINDING_REGISTER("Tablet", "StrokeColour", StrokeColour);
CONFIG_BINDING_END();

void FcitxTabletSaveConfig(FcitxTabletConfig* cfg) {
	FcitxConfigFileDesc *configDesc = GetFcitxTabletConfigDesc();
	FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-tablet.config", "w", NULL);
	if(fp) {
		FcitxConfigSaveConfigFileFp(fp, &cfg->config, configDesc);
		fclose(fp);
	}
}

boolean FcitxTabletLoadConfig(FcitxTabletConfig* cfg) {
	FcitxConfigFileDesc *configDesc = GetFcitxTabletConfigDesc();
	if(configDesc == NULL)
		return false;

	FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-tablet.config", "r", NULL);

	if (!fp && errno == ENOENT) {
		// Create the file if it doesn't exist
		FcitxTabletSaveConfig(cfg);
	}

	FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, configDesc);
	FcitxTabletConfigConfigBind(cfg, cfile, configDesc);
	FcitxConfigBindSync(&cfg->config);

	if(fp)
		fclose(fp);

	return true;
}

