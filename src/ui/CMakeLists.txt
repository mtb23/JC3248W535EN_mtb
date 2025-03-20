SET(SOURCES screens/ui_Screen_ScreenPraca.c
    screens/ui_Screen_ScreenUstawienia.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c)

add_library(ui ${SOURCES})
