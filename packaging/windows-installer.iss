#ifndef FluidSimVersion
  #define FluidSimVersion "dev"
#endif

[Setup]
AppId={{ea317e77-f60c-462c-9363-1c763bf4a7ad}}
AppName=FluidSimulation
AppVersion={#FluidSimVersion}
DefaultDirName={autopf}\FluidSimulation
DefaultGroupName=FluidSimulation
OutputDir=..
OutputBaseFilename=FluidSimulation-Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64

[Files]
Source: "..\dist\FluidSimulation\*"; DestDir: "{app}"; Flags: recursesubdirs

[Icons]
Name: "{group}\FluidSimulation"; Filename: "{app}\FluidSimulation.exe"
Name: "{commondesktop}\FluidSimulation"; Filename: "{app}\FluidSimulation.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"