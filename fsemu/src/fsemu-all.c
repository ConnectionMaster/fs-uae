#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------

#include "../manymouse/linux_evdev.c"
#include "../manymouse/macosx_hidmanager.c"
#include "../manymouse/macosx_hidutilities.c"
#include "../manymouse/manymouse.c"
#include "../manymouse/windows_wminput.c"
#include "../manymouse/x11_xinput2.c"

// ----------------------------------------------------------------------------

#include "fsemu-action.c"
#include "fsemu-alsaaudio.c"
#include "fsemu-application.c"
#include "fsemu-audio.c"
#include "fsemu-audiobuffer.c"
#include "fsemu-background.c"
#include "fsemu-common.c"
#include "fsemu-config.c"
#include "fsemu-control.c"
#include "fsemu-controller.c"
#include "fsemu-data.c"
#include "fsemu-emuthread.c"
#include "fsemu-events.c"
#include "fsemu-fade.c"
#include "fsemu-font.c"
#include "fsemu-fontcache.c"
#include "fsemu-frame.c"
#include "fsemu-frameinfo.c"
#include "fsemu-gamemode.c"
#include "fsemu-glvideo.c"
#include "fsemu-gui.c"
#include "fsemu-helpbar.c"
#include "fsemu-helper.c"
#include "fsemu-hud.c"
#include "fsemu-image.c"
#include "fsemu-inifile.c"
#include "fsemu-input.c"
#include "fsemu-inputdevice.c"
#include "fsemu-inputmode.c"
#include "fsemu-inputport.c"
#include "fsemu-layout.c"
#include "fsemu-led.c"
#include "fsemu-leds.c"
#include "fsemu-log.c"
#include "fsemu-main.c"
#include "fsemu-mainmenu.c"
#include "fsemu-manymouse.c"
#include "fsemu-media.c"
#include "fsemu-menu.c"
#include "fsemu-module.c"
#include "fsemu-monitor.c"
#include "fsemu-mouse.c"
#include "fsemu-mutex.c"
#include "fsemu-opengl.c"
#include "fsemu-option.c"
#include "fsemu-oskeyboard.c"
#include "fsemu-osmenu.c"
#include "fsemu-path.c"
#include "fsemu-perfgui.c"
#include "fsemu-quit.c"
#include "fsemu-recording.c"
#include "fsemu-refable.c"
#include "fsemu-savestate.c"
#include "fsemu-screenshot.c"
#include "fsemu-sdlaudio.c"
#include "fsemu-sdlgamecontrollerdb.c"
#include "fsemu-sdlinput.c"
#include "fsemu-sdlvideo.c"
#include "fsemu-sdlwindow.c"
#include "fsemu-shader.c"
#include "fsemu-startupinfo.c"
#include "fsemu-stream.c"
#include "fsemu-theme.c"
#include "fsemu-thread.c"
#include "fsemu-time.c"
#include "fsemu-titlebar.c"
#include "fsemu-util.c"
#include "fsemu-video.c"
#include "fsemu-videothread.c"
#include "fsemu-widget.c"
#include "fsemu-window.c"
#include "fsemu-xmlshader.c"
#include "fsemu.c"

// ----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif
