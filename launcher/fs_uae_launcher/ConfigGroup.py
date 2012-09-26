from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

import os
import fs_uae_launcher.fsui as fsui
from .Config import Config
from .IconButton import IconButton
from .Settings import Settings
from .I18N import _, ngettext

class ConfigGroup(fsui.Group):

    def __init__(self, parent):
        fsui.Group.__init__(self, parent)
        self.layout = fsui.VerticalLayout()

        heading_label = fsui.HeadingLabel(self, _("Configuration"))
        self.layout.add(heading_label, margin=10)
        self.layout.add_spacer(0)

        hori_layout = fsui.HorizontalLayout()
        self.layout.add(hori_layout, fill=True)

        self.new_button = IconButton(self, "new_button.png")
        self.new_button.set_tooltip(_("New Configuration"))
        self.new_button.on_activate = self.on_new_button
        hori_layout.add(self.new_button, margin=10)

        #self.open_button = IconButton(self, "open_button.png")
        #self.open_button.set_tooltip(_("Open Configuration"))
        #self.open_button.disable()
        #self.open_button.on_activate = self.on_open_button
        #hori_layout.add(self.open_button, margin=10)

        self.config_name_field = fsui.TextField(self)
        hori_layout.add(self.config_name_field, expand=True, margin=10)

        self.save_button = IconButton(self, "save_button.png")
        self.save_button.disable()
        self.save_button.set_tooltip(_("Save Configuration"))
        self.save_button.on_activate = self.on_save_button
        hori_layout.add(self.save_button, margin=10)

        self.on_setting("config_name", Settings.get("config_name"))
        self.config_name_field.on_change = self.on_config_name_change
        Settings.add_listener(self)

    def on_destroy(self):
        Settings.remove_listener(self)

    def on_setting(self, key, value):
        if key == "config_name":
            if value != self.config_name_field.get_text():
                self.config_name_field.set_text(value)

    def on_config_name_change(self):
        text = self.config_name_field.get_text().strip()
        Settings.set("config_name", text)
        # FIXME: remove
        Config.set("title", text)

    def on_new_button(self):
        Config.load_default_config()

    def on_save_button(self):
        pass
