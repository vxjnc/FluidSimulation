import json
import os
import urllib.request

import fluidsim.system as fsys
import fluidsim.ui as fui

REPO = "vxjnc/FluidSimulation"
PLUGINS_PATH = "plugins"


def list_remote_plugins():
    url = f"https://api.github.com/repos/{REPO}/contents/{PLUGINS_PATH}"
    with urllib.request.urlopen(url, timeout=10) as resp:
        data = json.load(resp)
    return [item["name"] for item in data if item["type"] == "dir"]


def install_plugin(name: str):
    url = f"https://api.github.com/repos/{REPO}/contents/{PLUGINS_PATH}/{name}"
    with urllib.request.urlopen(url, timeout=10) as resp:
        files = json.load(resp)

    target_dir = os.path.join(PLUGINS_PATH, name)
    os.makedirs(target_dir, exist_ok=True)

    for f in files:
        if f["type"] != "file":
            continue
        with urllib.request.urlopen(f["download_url"], timeout=10) as resp:
            content = resp.read()
        with open(os.path.join(target_dir, f["name"]), "wb") as out:
            out.write(content)

    fsys.notify_info(f"Plugin {name} installed")
    print(f"Installed: {name}")


def make_install_callback(name: str):
    def on_click(_):
        try:
            install_plugin(name)
        except Exception as e:
            print(f"Failed to install {name}: {e}")

    return on_click


def refresh(_state=None):
    try:
        names = list_remote_plugins()
    except Exception as e:
        print(f"Failed to fetch plugin list: {e}")
        return

    installed = set(os.listdir(PLUGINS_PATH)) if os.path.isdir(PLUGINS_PATH) else set()

    new_panel = fui.Panel()
    new_panel.title = "Plugin Installer"
    for name in names:
        label = f"Reinstall {name}" if name in installed else f"Install {name}"
        new_panel.add_button(f"install_{name}", label, make_install_callback(name))

    new_panel.add_button("refresh", "Refresh plugin list", refresh)
    fui.set_panel(new_panel)


panel = fui.Panel()
panel.title = "Plugin Installer"
panel.add_button("refresh", "Refresh plugin list", refresh)
fui.set_panel(panel)
