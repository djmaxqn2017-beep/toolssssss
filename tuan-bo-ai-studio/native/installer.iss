#define AppName "TBRetoch"
[Setup]
AppId={{83A238E8-EC89-4C9E-88A5-B765DF0AE036}
AppName={#AppName}
AppVersion=0.7.0
AppPublisher=TB
DefaultDirName={localappdata}\Programs\TBRetoch
DefaultGroupName=TBRetoch
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=dist
OutputBaseFilename=TBRetoch-Native-Core-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
LanguageDetectionMethod=none
UninstallDisplayIcon={app}\TBRetoch.exe
[Languages]
Name: "vietnamese"; MessagesFile: "Vietnamese.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"
[Files]
Source: "package\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{autoprograms}\TBRetoch"; Filename: "{app}\TBRetoch.exe"
Name: "{autodesktop}\TBRetoch"; Filename: "{app}\TBRetoch.exe"
[Run]
Filename: "{app}\TBRetoch.exe"; Description: "{cm:LaunchProgram,TBRetoch}"; Flags: nowait postinstall skipifsilent
