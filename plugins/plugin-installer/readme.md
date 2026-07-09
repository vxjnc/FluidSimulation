# Plugin Installer
Fetches available plugins from the [FluidSimulation](https://github.com/vxjnc/FluidSimulation) repository's `plugins` folder and installs them locally.
## Usage
1. Run the script — a "Plugin Installer" panel appears with a **Refresh plugin list** button.
2. Click it to fetch the list of available plugins from GitHub and show an **Install** button for each one.
3. Click **Install \<name\>** to download the plugin's files into `plugins/<name>/`.
## Notes
- Requires an internet connection.
- Re-installing a plugin overwrites its existing files.
- No update checking — installing again always re-downloads the latest version from the repo.