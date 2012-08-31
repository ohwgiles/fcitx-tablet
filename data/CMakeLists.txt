fcitx_add_addon_conf_file(fcitx-tablet-ime.conf)
fcitx_add_addon_conf_file(fcitx-tablet.conf)
fcitx_add_configdesc_file(fcitx-tablet.desc)
fcitx_add_inputmethod_conf_file(tablet.conf)
install(FILES tablet.png DESTINATION share/fcitx/skin/default/)
install(FILES tablet.png DESTINATION share/fcitx/skin/classic/)
install(FILES tablet.png DESTINATION share/fcitx/skin/dark/)

