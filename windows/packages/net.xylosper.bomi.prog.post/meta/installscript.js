function Component() {
}

Component.prototype.createOperations = function() {
    component.createOperations();

    var prog = installer.componentByName("net.xylosper.bomi.prog")
    component.addOperation("CreateShortcut", "@TargetDir@\\bomi.exe",
                           "@StartMenuDir@\\%1.lnk".arg(prog.displayName),
                           "workingDirectory=@TargetDir@");
    component.addOperation("CreateShortcut", "@TargetDir@\\maintenancetool.exe",
                           "@StartMenuDir@\\Update bomi.lnk",
                           "workingDirectory=@TargetDir@",
                           "--updater");
    component.addOperation("CreateShortcut", "@TargetDir@\\maintenancetool.exe",
                           "@StartMenuDir@\\Uninstall bomi.lnk",
                           "workingDirectory=@TargetDir@");
    if (installer.isUninstaller())
        component.addElevatedOperation("Execute", "@TargetDir@/bomi.exe", "--win-unassoc")
    else if (installer.isInstaller() && prog.userInterface("TargetWidget").assoc.checked)
        component.addElevatedOperation("Execute", "@TargetDir@/bomi.exe", "--win-assoc-default")
}
