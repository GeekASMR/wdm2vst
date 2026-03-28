; WDM2VST Public Edition - Installer Script
; Inno Setup 6 - 中文版

#define MyAppName "WDM2VST 虚拟音频路由"
#define MyAppVersion "3.2.2"
#define MyAppPublisher "VirtualAudioRouter"
#define MyAppURL "https://github.com/VirtualAudioRouter/WDM2VST"

[Setup]
AppId={{B4A7F2E1-3C5D-4E6F-8A9B-1C2D3E4F5A6B}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableDirPage=yes
DisableProgramGroupPage=yes
CloseApplications=force
CloseApplicationsFilter=*.vst3,*.dll
RestartApplications=no
UsePreviousAppDir=no
OutputDir=D:\Autigravity\wdm2vst\ASMRTOP_Plugins\installer
OutputBaseFilename=WDM2VST_Public_v{#MyAppVersion}_Setup
Compression=lzma2/ultra64
SolidCompression=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
DisableWelcomePage=no
DisableFinishedPage=no
WizardImageFile=D:\Autigravity\wdm2vst\ASMRTOP_Plugins\installer\wizard_image.bmp
WizardSmallImageFile=D:\Autigravity\wdm2vst\ASMRTOP_Plugins\installer\wizard_small.bmp
SetupIconFile=D:\Autigravity\wdm2vst\ASMRTOP_Plugins\installer\wdm2vst.ico
UninstallDisplayName={#MyAppName}
VersionInfoVersion={#MyAppVersion}.0
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription=WDM2VST 虚拟音频路由插件套件
VersionInfoProductName={#MyAppName}
LicenseFile=

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
; 覆盖所有界面文字为中文
SetupAppTitle=安装 - {#MyAppName}
SetupWindowTitle=安装 - {#MyAppName} v{#MyAppVersion}
WelcomeLabel1=欢迎安装 {#MyAppName}
WelcomeLabel2=即将安装 {#MyAppName} v{#MyAppVersion} 到您的电脑。%n%n本套件包含以下 VST3 插件：%n%n  • WDM2VST — 将系统声音捕获到 DAW%n  • VST2WDM — 将 DAW 声音输出到系统设备%n  • Audio Send — 跨插件音频发送%n  • Audio Receive — 跨插件音频接收%n%n【⚠️ 重要提示：设备管理器报错 37/52 解决方法】%n如果安装后驱动加载失败，请进入 Windows 安全中心 -> 设备安全性 -> 内核隔离，关闭【内存完整性】后重启电脑即可。%n%n点击"下一步"继续。
ClickNext=点击"下一步"继续，或点击"取消"退出安装。
SelectComponentsLabel2=选择要安装的组件，然后点击"下一步"继续。
SelectComponentsDesc=请选择要安装的插件组件
ComponentsDiskSpaceMBLabel=当前选择至少需要 [mb] MB 的磁盘空间。
FullInstallation=完整安装（全部 4 个插件）
CompactInstallation=精简安装（仅核心插件）
CustomInstallation=自定义安装
ReadyLabel1=准备安装
ReadyLabel2a=点击"安装"开始安装 {#MyAppName}，或点击"上一步"修改设置。
ReadyLabel2b=点击"安装"开始安装。
PreparingDesc=安装程序正在准备安装 {#MyAppName}。
ButtonNext=下一步 >
ButtonInstall=安装
ButtonBack=< 上一步
ButtonCancel=取消
ButtonFinish=完成
ClickFinish=点击"完成"退出安装向导。
FinishedHeadingLabel=安装完成
FinishedLabel={#MyAppName} v{#MyAppVersion} 已成功安装到您的电脑。%n%n所有 VST3 插件已安装到标准目录，请在 DAW 中扫描 VST3 插件即可使用。
StatusExtractFiles=正在解压文件...
InstallingLabel=正在安装 {#MyAppName}，请稍候...
ExitSetupTitle=退出安装
ExitSetupMessage=安装尚未完成。确定要退出吗？
SelectDirLabel3=将安装到以下目录：
DiskSpaceWarning=至少需要 %1 KB 的可用磁盘空间，但所选驱动器仅有 %2 KB 可用。%n%n是否继续？
DirExists=目录 %n%n%1%n%n 已存在。是否安装到此目录？
SelectTasksLabel2=选择安装时要执行的附加任务：
WizardSelectComponents=选择组件
SelectComponentsLabel3=请选择您想安装的组件，取消不需要的组件。

[Types]
Name: "full"; Description: "完整安装（全部 4 个插件）"
Name: "compact"; Description: "精简安装（仅核心插件）"
Name: "custom"; Description: "自定义安装"; Flags: iscustom

[Components]
Name: "wdm2vst"; Description: "WDM2VST — 系统声音捕获到 DAW（核心）"; Types: full compact custom; Flags: fixed
Name: "vst2wdm"; Description: "VST2WDM — DAW 声音输出到系统（核心）"; Types: full compact custom; Flags: fixed
Name: "send"; Description: "Audio Send — 跨插件音频发送（扩展）"; Types: full
Name: "recv"; Description: "Audio Receive — 跨插件音频接收（扩展）"; Types: full
Name: "inst"; Description: "INST WDM2VST — 音源发生器（扩展选装）"

[Files]

; Driver Setup Files
Source: "D:\Autigravity\sgin\签名\共存版已签名\gongkai\VirtualAudioRouter.sys"; DestDir: "{app}\Driver"; Flags: ignoreversion
Source: "D:\Autigravity\sgin\签名\共存版已签名\gongkai\VirtualAudioRouter.inf"; DestDir: "{app}\Driver"; Flags: ignoreversion
Source: "D:\Autigravity\sgin\签名\共存版已签名\gongkai\VirtualAudioRouter.cat"; DestDir: "{app}\Driver"; Flags: ignoreversion
Source: "D:\Autigravity\sgin\wdm2vst\gongkai\time.reg"; DestDir: "{tmp}"; Flags: ignoreversion deleteafterinstall
Source: "D:\Autigravity\sgin\wdm2vst\gongkai\devcon.exe"; DestDir: "{app}\Driver"; Flags: ignoreversion
Source: "D:\Autigravity\sgin\wdm2vst\gongkai\install.bat"; DestDir: "{tmp}"; Flags: ignoreversion deleteafterinstall
Source: "D:\Autigravity\sgin\wdm2vst\gongkai\uninstall.bat"; DestDir: "{tmp}"; Flags: ignoreversion deleteafterinstall
; WDM2VST plugin
Source: "D:\Autigravity\wdm2vst\ASMRTOP_Plugins\build_public\ASMRTOP_WDM2VST_artefacts\Release\VST3\WDM2VST.vst3"; DestDir: "{commonpf}\Common Files\VST3\VirtualAudioRouter"; Components: wdm2vst; Flags: ignoreversion

; VST2WDM plugin
Source: "D:\Autigravity\wdm2vst\ASMRTOP_Plugins\build_public\ASMRTOP_VST2WDM_artefacts\Release\VST3\VST2WDM.vst3"; DestDir: "{commonpf}\Common Files\VST3\VirtualAudioRouter"; Components: vst2wdm; Flags: ignoreversion

; Audio Send plugin
Source: "D:\Autigravity\wdm2vst\ASMRTOP_Plugins\build_public\ASMRTOP_SEND_artefacts\Release\VST3\Audio Send.vst3"; DestDir: "{commonpf}\Common Files\VST3\VirtualAudioRouter"; Components: send; Flags: ignoreversion

; Audio Receive plugin
Source: "D:\Autigravity\wdm2vst\ASMRTOP_Plugins\build_public\ASMRTOP_RECV_artefacts\Release\VST3\Audio Receive.vst3"; DestDir: "{commonpf}\Common Files\VST3\VirtualAudioRouter"; Components: recv; Flags: ignoreversion

; INST WDM2VST plugin
Source: "D:\Autigravity\wdm2vst\ASMRTOP_Plugins\build_public\ASMRTOP_INST_WDM2VST_artefacts\Release\VST3\INST WDM2VST.vst3"; DestDir: "{commonpf}\Common Files\VST3\VirtualAudioRouter"; Components: inst; Flags: ignoreversion

; Documentation
Source: "E:\Antigravity\成品开发\WDM2VST\WDM2VST_介绍.html"; DestDir: "{app}"; DestName: "WDM2VST_介绍.html"; Flags: ignoreversion

[InstallDelete]
Type: filesandordirs; Name: "{commonpf}\Common Files\VST3\VirtualAudioRouter\*.*"
Type: filesandordirs; Name: "{commonpf}\Common Files\VST3\WDM2VST.vst3"
Type: filesandordirs; Name: "{commonpf}\Common Files\VST3\VST2WDM.vst3"
Type: filesandordirs; Name: "{commonpf}\Common Files\VST3\Audio Send.vst3"
Type: filesandordirs; Name: "{commonpf}\Common Files\VST3\Audio Receive.vst3"
Type: filesandordirs; Name: "{commonpf}\Common Files\VST3\INST WDM2VST.vst3"
Type: filesandordirs; Name: "{commonpf}\Common Files\VST3\ASMRTOP WDM2VST.vst3"
Type: filesandordirs; Name: "{commonpf}\Common Files\VST3\ASMRTOP VST2WDM.vst3"
Type: filesandordirs; Name: "{commonpf}\Common Files\VST3\ASMRTOP Send.vst3"
Type: filesandordirs; Name: "{commonpf}\Common Files\VST3\ASMRTOP Receive.vst3"
Type: filesandordirs; Name: "{commonpf}\Common Files\VST3\ASMRTOP INST WDM2VST.vst3"

[UninstallDelete]
Type: filesandordirs; Name: "{commonpf}\Common Files\VST3\VirtualAudioRouter"
Type: filesandordirs; Name: "{commonpf}\Common Files\VST3\WDM2VST.vst3"
Type: filesandordirs; Name: "{commonpf}\Common Files\VST3\VST2WDM.vst3"
Type: filesandordirs; Name: "{commonpf}\Common Files\VST3\Audio Send.vst3"
Type: filesandordirs; Name: "{commonpf}\Common Files\VST3\Audio Receive.vst3"
Type: filesandordirs; Name: "{commonpf}\Common Files\VST3\INST WDM2VST.vst3"
Type: files; Name: "{app}\WDM2VST_介绍.html"


[Tasks]
Name: "telemetry"; Description: "允许发送匿名诊断和崩溃数据以帮助改进 WDM2VST 稳定性分析"; GroupDescription: "隐私与体验改进:"; Flags: checkedonce

[INI]
Filename: "{commonpf}\Common Files\VST3\VirtualAudioRouter\config.ini"; Section: "Settings"; Key: "EnableTelemetry"; String: "1"; Tasks: telemetry

[Run]
Filename: "{win}\regedit.exe"; Parameters: "/s ""{tmp}\time.reg"""; StatusMsg: "Configuring environment..."; Flags: runhidden waituntilterminated
Filename: "{sys}\pnputil.exe"; Parameters: "/add-driver ""{app}\Driver\VirtualAudioRouter.inf"" /install"; StatusMsg: "Registering driver..."; Flags: runhidden waituntilterminated
Filename: "{app}\Driver\devcon.exe"; Parameters: "install ""{app}\Driver\VirtualAudioRouter.inf"" ROOT\VirtualAudioRouter"; StatusMsg: "Installing Virtual Audio Driver (This might take a moment)..."; Flags: runhidden waituntilterminated

[UninstallRun]
Filename: "{app}\Driver\devcon.exe"; Parameters: "remove ROOT\VirtualAudioRouter"; Flags: runhidden waituntilterminated; RunOnceId: "UninstallDriver"
Filename: "{sys}\pnputil.exe"; Parameters: "/delete-driver VirtualAudioRouter.inf /uninstall"; Flags: runhidden waituntilterminated; RunOnceId: "UninstallDriverPkg"

[Code]
function InitializeSetup(): Boolean;
var
  UninstallKey: String;
  UninstallStr: String;
  ResultCode: Integer;
begin
  Result := True;
  // 静默卸载旧版本
  UninstallKey := 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{B4A7F2E1-3C5D-4E6F-8A9B-1C2D3E4F5A6B}_is1';
  if RegQueryStringValue(HKLM, UninstallKey, 'UninstallString', UninstallStr) then
  begin
    UninstallStr := RemoveQuotes(UninstallStr);
    Exec(UninstallStr, '/VERYSILENT /NORESTART /SUPPRESSMSGBOXES', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  HtmlPath: String;
  ErrorCode: Integer;
begin
  if CurStep = ssPostInstall then
  begin
    Log('VST3 插件已成功安装到: ' + ExpandConstant('{app}'));
    ShellExec('open', 'https://geek.asmrtop.cn/wdm2vst.html', '', '', SW_SHOWNORMAL, ewNoWait, ErrorCode);
  end;
end;
