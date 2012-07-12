#include "config.h"

#include <fcitx/instance.h>
#include <fcitx-utils/log.h>
#include <fcitx-config/xdg.h>
#include <errno.h>

CONFIG_DESC_DEFINE(GetFcitxTabletConfigDesc, "fcitx-tablet.desc")

CONFIG_BINDING_BEGIN(FcitxTabletConfig);
CONFIG_BINDING_REGISTER("Tablet", "DevicePath", devicePath);
CONFIG_BINDING_END();


void FcitxTabletSaveConfig(FcitxTabletConfig* cfg) {
	FcitxConfigFileDesc *configDesc = GetFcitxTabletConfigDesc();
		 FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-tablet.config", "w", NULL);
		 FcitxConfigSaveConfigFileFp(fp, &cfg->config, configDesc);
		 if (fp)
			  fclose(fp);
}

boolean FcitxTabletLoadConfig(FcitxTabletConfig* cfg) {
	 FcitxConfigFileDesc *configDesc = GetFcitxTabletConfigDesc();
	 if (configDesc == NULL)
		  return false;

	 FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-tablet.config", "r", NULL);

	 if (!fp)
	 {
		  if (errno == ENOENT)
				FcitxTabletSaveConfig(cfg);
	 }
	 FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, configDesc);
	 FcitxTabletConfigConfigBind(cfg, cfile, configDesc);
	 FcitxConfigBindSync(&cfg->config);

	 if (fp)
		  fclose(fp);

	 //LoadLayoutOverride(xkb);

	 return true;
}

