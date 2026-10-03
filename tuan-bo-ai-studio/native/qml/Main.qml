import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: window
    width: 1540
    height: 940
    minimumWidth: 1180
    minimumHeight: 720
    visible: true
    title: "TBRetoch"
    color: "#0d0c10"
    palette.window: "#17151b"
    palette.windowText: "#e9e5ee"
    palette.base: "#211e25"
    palette.alternateBase: "#28242e"
    palette.text: "#e9e5ee"
    palette.button: "#29242f"
    palette.buttonText: "#e9e5ee"
    palette.highlight: "#8247d6"
    palette.highlightedText: "#ffffff"

    property string viewMode: "after"
    property real zoom: 1.0
    property real panX: 0
    property real panY: 0
    property string toastText: ""
    property string lang: i18n.language

    function trKey(key) {
        var dependency = lang
        return i18n.t(key)
    }

    function resetView() {
        if (sourceImage.status !== Image.Ready || sourceImage.implicitWidth <= 0 || sourceImage.implicitHeight <= 0) return
        var z = Math.min((viewer.width - 56) / sourceImage.implicitWidth,
                         (viewer.height - 56) / sourceImage.implicitHeight)
        zoom = Math.min(1.0, Math.max(0.05, z))
        panX = 0
        panY = 0
    }

    function zoomTo(value) {
        zoom = Math.max(0.05, Math.min(8.0, value))
    }

    FileDialog {
        id: openDialog
        title: trKey("dialog.addImages")
        fileMode: FileDialog.OpenFiles
        nameFilters: [trKey("filter.images"), trKey("filter.all")]
        onAccepted: appController.importFiles(selectedFiles)
    }

    FolderDialog {
        id: exportFolderDialog
        title: trKey("dialog.chooseExportFolder")
        onAccepted: appController.exportCurrent(selectedFolder, formatBox.currentText, Math.round(qualitySlider.value))
    }

    Connections {
        target: appController
        function onExportFinished(path, bytes, width, height) {
            toastText = trKey("status.exportDone")
                .replace("%1", width)
                .replace("%2", height)
                .replace("%3", (bytes / 1024 / 1024).toFixed(2)) + "\n" + path
            toastTimer.restart()
        }
        function onErrorOccurred(message) {
            toastText = trKey(message)
            toastTimer.restart()
        }
        function onCurrentImageChanged() { Qt.callLater(resetView) }
    }

    Timer { id: toastTimer; interval: 4200; onTriggered: toastText = "" }

    Shortcut { sequence: "Ctrl+0"; onActivated: resetView() }
    Shortcut { sequence: "Ctrl+1"; onActivated: zoomTo(1.0) }
    Shortcut { sequence: "Ctrl+Z"; onActivated: appController.undo() }
    Shortcut { sequence: "Ctrl+Y"; onActivated: appController.redo() }
    Shortcut { sequence: "Space"; onActivated: viewMode = viewMode === "before" ? "after" : "before" }
    Shortcut { sequence: "Ctrl+C"; onActivated: appController.copySettings() }
    Shortcut { sequence: "Ctrl+V"; onActivated: appController.pasteSettings() }
    Shortcut { sequence: "Ctrl+Shift+E"; onActivated: if (appController.currentIndex >= 0) exportFolderDialog.open() }
    Shortcut { sequence: "\\"; onActivated: viewMode = viewMode === "before" ? "after" : "before" }

    header: Rectangle {
        height: 56
        color: "#17151b"
        border.color: "#2b2731"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            spacing: 7

            Rectangle {
                width: 36; height: 36; radius: 10
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#256fd0" }
                    GradientStop { position: 1.0; color: "#7334df" }
                }
                Label { anchors.centerIn: parent; text: "TB"; color: "white"; font.bold: true; font.italic: true; font.pixelSize: 15 }
            }

            Label { text: "TBRetoch"; color: "white"; font.bold: true; font.pixelSize: 14 }

            ToolSeparator {}
            Button { text: trKey("nav.library") }
            Button { text: trKey("nav.edit"); highlighted: true }
            Button { text: trKey("nav.compare") }
            Button { text: trKey("nav.ai") }
            Button { text: trKey("nav.sync") }

            ToolSeparator {}
            Button { text: trKey("action.addImages"); onClicked: openDialog.open() }
            Button { text: trKey("action.copy"); enabled: appController.currentIndex >= 0; onClicked: appController.copySettings() }
            Button { text: trKey("action.paste"); enabled: appController.currentIndex >= 0; onClicked: appController.pasteSettings() }
            Button { text: trKey("action.reset"); enabled: appController.currentIndex >= 0; onClicked: appController.resetCurrentSettings() }

            Item { Layout.fillWidth: true }

            Label { text: trKey(appController.statusText); color: "#cfa5ff"; font.pixelSize: 10 }
            BusyIndicator { running: appController.busy; visible: running; implicitWidth: 24; implicitHeight: 24 }

            ComboBox {
                id: languageBox
                Layout.preferredWidth: 125
                model: ["Tiếng Việt", "English"]
                currentIndex: i18n.language === "en" ? 1 : 0
                onActivated: i18n.language = currentIndex === 1 ? "en" : "vi"
            }

            Button {
                text: trKey("action.export")
                enabled: appController.currentIndex >= 0 && !appController.busy
                highlighted: true
                onClicked: exportFolderDialog.open()
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 210
            Layout.fillHeight: true
            color: "#141217"
            border.color: "#2a2630"

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                TabBar {
                    Layout.fillWidth: true
                    TabButton { text: trKey("section.mask") }
                    TabButton { text: trKey("action.undo") + "/" + trKey("action.redo") }
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    ColumnLayout {
                        width: parent.width
                        spacing: 8
                        anchors.margins: 10
                        Label { text: trKey("section.mask"); color: "white"; font.bold: true }
                        Repeater {
                            model: ["mask.subject", "mask.person", "mask.faceSkin", "mask.bodySkin", "section.hair", "section.clothing", "section.background", "section.eyes", "mask.lips", "mask.teeth"]
                            delegate: Button { required property string modelData; text: trKey(modelData); Layout.fillWidth: true; enabled: false }
                        }
                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            text: trKey("info.semanticPending")
                            color: "#8e8796"
                            font.pixelSize: 10
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#09090b"

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    color: "#131116"
                    border.color: "#26222b"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        Label {
                            text: appController.currentName.length ? appController.currentName : trKey("viewer.noImage")
                            color: "#d2ccd8"
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                        Button { text: trKey("action.after"); checkable: true; checked: viewMode === "after"; onClicked: viewMode = "after" }
                        Button { text: trKey("action.before"); checkable: true; checked: viewMode === "before"; onClicked: viewMode = "before" }
                        Button { text: "A/B"; checkable: true; checked: viewMode === "split"; onClicked: viewMode = "split" }
                        Button { text: Math.round(zoom * 100) + "%"; onClicked: resetView() }
                    }
                }

                Item {
                    id: viewer
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    Rectangle { anchors.fill: parent; color: "#09090b" }

                    Item {
                        id: photoLayer
                        objectName: "photoLayer"
                        width: sourceImage.implicitWidth
                        height: sourceImage.implicitHeight
                        x: (viewer.width - width) / 2 + panX
                        y: (viewer.height - height) / 2 + panY
                        scale: zoom
                        transformOrigin: Item.Center
                        visible: appController.currentIndex >= 0

                        Image {
                            id: sourceImage
                            anchors.fill: parent
                            source: appController.currentPreviewUrl
                            asynchronous: true
                            cache: true
                            smooth: true
                            mipmap: true
                            visible: viewMode === "before"
                            onStatusChanged: if (status === Image.Ready) Qt.callLater(resetView)
                        }

                        ShaderEffect {
                            anchors.fill: parent
                            visible: viewMode !== "before"
                            property variant source: sourceImage
                            property real exposure: Number(appController.currentSettings.exposure ?? 0)
                            property real contrast: Number(appController.currentSettings.contrast ?? 0)
                            property real highlights: Number(appController.currentSettings.highlights ?? 0)
                            property real shadows: Number(appController.currentSettings.shadows ?? 0)
                            property real whites: Number(appController.currentSettings.whites ?? 0)
                            property real blacks: Number(appController.currentSettings.blacks ?? 0)
                            property real temperature: Number(appController.currentSettings.temperature ?? 0)
                            property real tint: Number(appController.currentSettings.tint ?? 0)
                            property real saturation: Number(appController.currentSettings.saturation ?? 0)
                            property real vibrance: Number(appController.currentSettings.vibrance ?? 0)
                            property real clarity: Number(appController.currentSettings.clarity ?? 0)
                            property real dehaze: Number(appController.currentSettings.dehaze ?? 0)
                            property real fade: Number(appController.currentSettings.fade ?? 0)
                            vertexShader: "qrc:/shaders/color.vert.qsb"
                            fragmentShader: "qrc:/shaders/color.frag.qsb"
                        }

                        Item {
                            visible: viewMode === "split"
                            width: parent.width / 2
                            height: parent.height
                            clip: true
                            z: 4
                            Image {
                                width: photoLayer.width
                                height: photoLayer.height
                                source: appController.currentPreviewUrl
                                asynchronous: true
                                smooth: true
                                mipmap: true
                            }
                        }

                        Rectangle {
                            visible: viewMode === "split"
                            x: parent.width / 2 - 1
                            width: 2
                            height: parent.height
                            color: "#b96fff"
                            z: 5
                        }
                    }

                    Column {
                        anchors.centerIn: parent
                        spacing: 10
                        visible: appController.currentIndex < 0
                        Rectangle {
                            width: 82; height: 82; radius: 22
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: "#2478d4" }
                                GradientStop { position: 1.0; color: "#6630d9" }
                            }
                            anchors.horizontalCenter: parent.horizontalCenter
                            Label { anchors.centerIn: parent; text: "TB"; color: "white"; font.pixelSize: 30; font.bold: true; font.italic: true }
                        }
                        Label { text: "TBRetoch"; color: "white"; font.pixelSize: 24; font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
                        Label { text: trKey("viewer.nativeOffline"); color: "#918b99"; anchors.horizontalCenter: parent.horizontalCenter }
                        Button { text: trKey("action.importImages"); anchors.horizontalCenter: parent.horizontalCenter; onClicked: openDialog.open() }
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        hoverEnabled: true
                        property real startMouseX
                        property real startMouseY
                        property real startPanX
                        property real startPanY
                        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                        onPressed: function(mouse) {
                            startMouseX = mouse.x
                            startMouseY = mouse.y
                            startPanX = panX
                            startPanY = panY
                        }
                        onPositionChanged: function(mouse) {
                            if (!pressed) return
                            panX = startPanX + (mouse.x - startMouseX)
                            panY = startPanY + (mouse.y - startMouseY)
                        }
                        onDoubleClicked: resetView()
                        onWheel: function(wheel) {
                            var oldZoom = zoom
                            var factor = wheel.angleDelta.y > 0 ? 1.12 : 0.89
                            var newZoom = Math.max(0.05, Math.min(8.0, zoom * factor))
                            if (newZoom !== oldZoom) {
                                var cx = viewer.width / 2 + panX
                                var cy = viewer.height / 2 + panY
                                panX = wheel.x - viewer.width / 2 - (wheel.x - cx) * (newZoom / oldZoom)
                                panY = wheel.y - viewer.height / 2 - (wheel.y - cy) * (newZoom / oldZoom)
                                zoom = newZoom
                            }
                            wheel.accepted = true
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 128
                    color: "#151318"
                    border.color: "#28242c"

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0
                        RowLayout {
                            Layout.fillWidth: true
                            height: 30
                            Layout.leftMargin: 8
                            Layout.rightMargin: 8
                            Label { text: trKey("viewer.imagesCount").replace("%1", appController.images.length); color: "#aaa3b0"; font.pixelSize: 10 }
                            Item { Layout.fillWidth: true }
                            Button { text: "‹"; enabled: appController.currentIndex > 0; onClicked: appController.selectImage(appController.currentIndex - 1) }
                            Button { text: "›"; enabled: appController.currentIndex >= 0 && appController.currentIndex < appController.images.length - 1; onClicked: appController.selectImage(appController.currentIndex + 1) }
                        }
                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            orientation: ListView.Horizontal
                            spacing: 6
                            clip: true
                            model: appController.images
                            delegate: Rectangle {
                                required property var modelData
                                width: 112; height: 84; radius: 5
                                color: "#211e25"
                                border.width: modelData.index === appController.currentIndex ? 2 : 1
                                border.color: modelData.index === appController.currentIndex ? "#b663ff" : "#3a3440"
                                Image { anchors.fill: parent; anchors.margins: 2; source: modelData.thumbUrl; fillMode: Image.PreserveAspectCrop; asynchronous: true; cache: true; smooth: true }
                                MouseArea { anchors.fill: parent; onClicked: appController.selectImage(modelData.index) }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.preferredWidth: 382
            Layout.fillHeight: true
            color: "#17151a"
            border.color: "#2e2933"

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                TabBar {
                    id: tabs
                    Layout.fillWidth: true
                    TabButton { text: trKey("tab.color") }
                    TabButton { text: trKey("tab.portrait") }
                    TabButton { text: trKey("tab.background") }
                    TabButton { text: trKey("tab.clothing") }
                    TabButton { text: trKey("tab.lighting") }
                    TabButton { text: trKey("tab.crop") }
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: tabs.currentIndex

                    ScrollView {
                        clip: true
                        ColumnLayout {
                            width: parent.width
                            spacing: 0

                            Rectangle {
                                Layout.fillWidth: true
                                height: 74
                                color: "#121015"
                                border.color: "#2d2832"
                                Column {
                                    anchors.centerIn: parent
                                    spacing: 4
                                    Label { text: trKey("info.gpuColor"); color: "#b36cff"; font.pixelSize: 9; font.bold: true }
                                    Label { text: trKey("info.gpuPreview"); color: "#8f8996"; font.pixelSize: 10 }
                                }
                            }

                            GroupBox {
                                title: trKey("section.basic")
                                Layout.fillWidth: true
                                ColumnLayout {
                                    width: parent.width
                                    EditSlider { title: trKey("control.exposure"); keyName: "exposure"; from: -3; to: 3; stepSize: 0.01; Layout.fillWidth: true }
                                    EditSlider { title: trKey("control.contrast"); keyName: "contrast"; Layout.fillWidth: true }
                                    EditSlider { title: trKey("control.highlights"); keyName: "highlights"; Layout.fillWidth: true }
                                    EditSlider { title: trKey("control.shadows"); keyName: "shadows"; Layout.fillWidth: true }
                                    EditSlider { title: trKey("control.whites"); keyName: "whites"; Layout.fillWidth: true }
                                    EditSlider { title: trKey("control.blacks"); keyName: "blacks"; Layout.fillWidth: true }
                                    EditSlider { title: trKey("control.temperature"); keyName: "temperature"; Layout.fillWidth: true }
                                    EditSlider { title: trKey("control.tint"); keyName: "tint"; Layout.fillWidth: true }
                                    EditSlider { title: trKey("control.vibrance"); keyName: "vibrance"; Layout.fillWidth: true }
                                    EditSlider { title: trKey("control.saturation"); keyName: "saturation"; Layout.fillWidth: true }
                                    EditSlider { title: trKey("control.clarity"); keyName: "clarity"; Layout.fillWidth: true }
                                    EditSlider { title: trKey("control.dehaze"); keyName: "dehaze"; Layout.fillWidth: true }
                                    EditSlider { title: trKey("control.fade"); keyName: "fade"; Layout.fillWidth: true }
                                }
                            }

                            GroupBox {
                                title: trKey("section.export")
                                Layout.fillWidth: true
                                ColumnLayout {
                                    width: parent.width
                                    Label { text: trKey("export.originalResolution"); color: "#9f96a8"; font.pixelSize: 10 }
                                    RowLayout {
                                        Label { text: trKey("export.format"); color: "#ddd8e3"; Layout.fillWidth: true }
                                        ComboBox { id: formatBox; model: ["jpg", "png", "webp"]; currentIndex: 0 }
                                    }
                                    RowLayout {
                                        Label { text: trKey("export.quality"); color: "#ddd8e3"; Layout.fillWidth: true }
                                        Label { text: Math.round(qualitySlider.value); color: "#c994ff" }
                                    }
                                    Slider { id: qualitySlider; from: 70; to: 100; value: 98; stepSize: 1; Layout.fillWidth: true }
                                }
                            }
                        }
                    }

                    ScrollView {
                        clip: true
                        ColumnLayout {
                            width: parent.width
                            GroupBox { title: trKey("section.skin"); Layout.fillWidth: true; ColumnLayout { width: parent.width; Repeater { model: ["control.blemishRemoval","control.skinSoftening","control.textureRecovery","control.faceShine","control.skinUnify","control.eyeBags","control.darkCircles","control.wrinkles","control.doubleChin"]; delegate: Button { required property string modelData; text: trKey(modelData); Layout.fillWidth: true; enabled: false } } } }
                            GroupBox { title: trKey("section.face"); Layout.fillWidth: true; ColumnLayout { width: parent.width; Repeater { model: ["control.faceWidth","control.jaw","control.chin","control.vShape","control.eyeSize","control.noseWidth","control.lipSize"]; delegate: Button { required property string modelData; text: trKey(modelData); Layout.fillWidth: true; enabled: false } } } }
                            GroupBox { title: trKey("section.eyes"); Layout.fillWidth: true; ColumnLayout { width: parent.width; Repeater { model: ["control.iris","control.eyeWhites","control.catchlight","control.teethWhitening"]; delegate: Button { required property string modelData; text: trKey(modelData); Layout.fillWidth: true; enabled: false } } } }
                            GroupBox { title: trKey("section.makeup"); Layout.fillWidth: true; ColumnLayout { width: parent.width; Repeater { model: ["control.lipstick","control.blush","control.eyeliner","control.eyeshadow","control.eyebrow"]; delegate: Button { required property string modelData; text: trKey(modelData); Layout.fillWidth: true; enabled: false } } } }
                        }
                    }

                    ScrollView {
                        clip: true
                        ColumnLayout {
                            width: parent.width
                            GroupBox { title: trKey("section.background"); Layout.fillWidth: true; ColumnLayout { width: parent.width; Repeater { model: ["control.bgCleanup","control.bgBlur","control.lensBlur","control.skyReplacement"]; delegate: Button { required property string modelData; text: trKey(modelData); Layout.fillWidth: true; enabled: false } } } }
                        }
                    }

                    ScrollView {
                        clip: true
                        ColumnLayout {
                            width: parent.width
                            GroupBox { title: trKey("section.clothing"); Layout.fillWidth: true; ColumnLayout { width: parent.width; Repeater { model: ["control.wrinkleRemoval","control.lintRemoval","control.stainRemoval"]; delegate: Button { required property string modelData; text: trKey(modelData); Layout.fillWidth: true; enabled: false } } } }
                        }
                    }

                    ScrollView {
                        clip: true
                        ColumnLayout {
                            width: parent.width
                            GroupBox { title: trKey("tab.lighting"); Layout.fillWidth: true; ColumnLayout { width: parent.width; Repeater { model: ["control.relight","control.subjectLight","control.rimLight","control.vignette"]; delegate: Button { required property string modelData; text: trKey(modelData); Layout.fillWidth: true; enabled: false } } } }
                        }
                    }

                    Rectangle {
                        color: "transparent"
                        Label {
                            anchors.centerIn: parent
                            width: parent.width - 40
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                            color: "#aaa3b0"
                            text: trKey("info.semanticPending")
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        visible: toastText.length > 0
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 150
        width: Math.min(parent.width - 60, 700)
        height: toastLabel.implicitHeight + 24
        radius: 8
        color: "#26212d"
        border.color: "#6e42a8"
        z: 100
        Label {
            id: toastLabel
            anchors.fill: parent
            anchors.margins: 12
            text: toastText
            wrapMode: Text.WordWrap
            color: "white"
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
