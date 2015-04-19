var Dir = new function () {
    this.toNative = function (path) {
        if (systemInfo.productType === "windows")
            return path.replace(/\//g, '\\');
        return path;
    }
};

function Component() {
    var b32 = installer.componentByName("net.xylosper.bomi.prog.i386");
    var b64 = installer.componentByName("net.xylosper.bomi.prog.x86_64");
    b32.setValue("Virtual", "true")
    b64.setValue("Virtual", "true")
    var ess = installer.componentByName("net.xylosper.bomi.prog.post");
    var native = systemInfo.currentCpuArchitecture !== "x86_64" ? b32 : b64
    native.setValue("Virtual", "false")
    native.setValue("Default", "true")
    native.enabled = false
    native.selectedChanged.connect(native, Component.prototype.checkComponent)
    if (installer.isInstaller()) {
        component.loaded.connect(this, Component.prototype.installerLoaded);
        installer.setDefaultPageVisible(QInstaller.TargetDirectory, false);
    }
}

Component.prototype.createOperations = function() {
    component.createOperations();
}

Component.prototype.installerLoaded = function () {
    if (installer.addWizardPage(component, "TargetWidget", QInstaller.TargetDirectory)) {
        var widget = gui.pageWidgetByObjectName("DynamicTargetWidget");
        if (widget) {
            widget.targetChooser.clicked.connect(this, Component.prototype.chooseTarget);
            widget.targetDirectory.textChanged.connect(this, Component.prototype.targetChanged);
            widget.windowTitle = "Installation Folder";
            widget.targetDirectory.text = Dir.toNative(installer.value("TargetDir"));
        }
    }
}

Component.prototype.targetChanged = function (text) {
    var widget = gui.pageWidgetByObjectName("DynamicTargetWidget");
    if (widget) {
        widget.complete = text.length > 0;
        installer.setValue("TargetDir", text);
    }
}

Component.prototype.chooseTarget = function () {
    var widget = gui.pageWidgetByObjectName("DynamicTargetWidget");
    if (widget) {
        var newTarget = QFileDialog.getExistingDirectory("Select Installation Folder",
                                                         widget.targetDirectory.text);
        if (newTarget.length > 0)
            widget.targetDirectory.text = Dir.toNative(newTarget);
    }
}

Component.prototype.checkComponent = function (sel) {
    var widget = gui.pageById(QInstaller.ComponentSelection);
    console.log(widget)
    __asd__;
    if (widget) {
        widget.complete = sel
    }
}
