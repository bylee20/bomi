function Controller() {
    var text = installer.value("TargetDir")
    if (text.indexOf("\\Program Files (x86)\\bomi") > -1)
        installer.setValue("TargetDir", text.replace("\\Program Files (x86)\\bomi", "\\Program Files\\bomi"))
}

var terminate = false

Controller.prototype.ComponentSelectionPageCallback = function() {
    var w = gui.currentPageWidget();
    if (!w)
        return
    w.SelectAllComponentsButton.visible = false
    w.DeselectAllComponentsButton.visible = false
    w.SelectDefaultComponentsButton.visible = false
    if (!installer.isInstaller())
        return;
    var dir = installer.value("TargetDir")
    if (!installer.fileExists(dir + "/bomi.exe")
            || !installer.fileExists(dir + "/maintenancetool.exe"))
        return;
    gui.clickButton(buttons.BackButton)
    if (QMessageBox.question("", "bomi - Multimedia Player",
                             "Previously installed version exists.\n"+
                             "Do you want to update or uninstall it?",
                             QMessageBox.Yes | QMessageBox.Cancel,
                             QMessageBox.Cancel) === QMessageBox.Cancel)
        return;
    if (!installer.executeDetached(dir + "/maintenancetool.exe", "--updater"))
        QMessageBox.critical("", "bomi - Multimedia Player", "Failed to launch maintenancetool.exe.")
    terminate = true
    installer.setValue("FinishedText", "<font color='red' size=3>The installer was quit.</font>");
    installer.setDefaultPageVisible(QInstaller.TargetDirectory, false);
    installer.setDefaultPageVisible(QInstaller.ReadyForInstallation, false);
    installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
    installer.setDefaultPageVisible(QInstaller.StartMenuSelection, false);
    installer.setDefaultPageVisible(QInstaller.PerformInstallation, false);
    installer.setDefaultPageVisible(QInstaller.LicenseCheck, false);
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.StartMenuDirectoryPageCallback = function() {
    var w = gui.currentPageWidget();
    if (w)
        w.StartMenuPathLineEdit.text = installer.componentByName("net.xylosper.bomi.prog").displayName
}

Controller.prototype.FinishedPageCallback = function() {
    var w = gui.currentPageWidget();
    if (w && terminate)
        gui.clickButton(buttons.FinishButton)
}
