#ifndef FCITX_TABLET_CONFIG_H
#define FCITX_TABLET_CONFIG_H
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

#include <fcitx-config/fcitx-config.h>

typedef struct _FcitxTabletConfig {
	FcitxGenericConfig config;
	// The path to the tablet device, e.g. /dev/hidraw0
	char* devicePath;
} FcitxTabletConfig;

boolean FcitxTabletLoadConfig(FcitxTabletConfig* cfg);

void FcitxTabletSaveConfig(FcitxTabletConfig* cfg);

#endif // FCITX_TABLET_CONFIG_H
