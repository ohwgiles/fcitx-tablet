#ifndef FCITX_TABLET_CONFIG_H
#define FCITX_TABLET_CONFIG_H

#include <fcitx-config/fcitx-config.h>

typedef struct _FcitxTabletConfig {
	FcitxGenericConfig config;
	char* devicePath;
} FcitxTabletConfig;

boolean FcitxTabletLoadConfig(FcitxTabletConfig* cfg);
	void FcitxTabletSaveConfig(FcitxTabletConfig* cfg);


#endif // CONFIG_H
