import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: window
    width: 1500
    height: 930
    minimumWidth: 1100
    minimumHeight: 700
    visible: true
    title: "TBRetoch"
    color: "#0e0d10"

    property string viewMode: "after"
    property real zoom: 1.0
    property real panX: 0
    property real panY: 0
    property string toastText: ""

    function resetView() {
        if (sourceImage.status !== Image.Ready || sourceImage.implicitWidth <= 0 || sourceImage.implicitHeight <= 0) return
        var z = Math.min((viewer.width - 56) / sourceImage.implicitWidth,
                         (viewer.height - 56) / sourceImage.implicitHeight)
        zoom = Math.min(1.0, Math.max(0.05, z))
        panX = 0
        panY = 0
    }

    FileDialog {
        id: openDialog
        title: "Thêm ảnh vào TBRetoch"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Ảnh (*.jpg *.jpeg *.png *.webp *.bmp *.tif *.tiff)", "Tất cả (*.*)"]
        onAccepted: appController.importFiles(selectedFiles)
    }

    FolderDialog {
        id: exportFolderDialog
        title: "Chọn thư mục xuất ảnh"
        onAccepted: appController.exportCurrent(selectedFolder, formatBox.currentText, Math.round(qualitySlider.value))
    }

    Connections {
        target: appController
        function onExportFinished(path, bytes, width, height) {
            toastText = "Đã xuất " + width + " × " + height + " • " + (bytes / 1024 / 1024).toFixed(2) + " MB\n" + path
            toastTimer.restart()
        }
        function onErrorOccurred(message) {
            toastText = message
            toastTimer.restart()
        }
        function onCurrentImageChanged() { Qt.callLater(resetView) }
    }

    Timer { id: toastTimer; interval: 4200; onTriggered: toastText = "" }

    header: Rectangle {
        height: 52
        color: "#17151a"
        border.color: "#2b2730"
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            spacing: 8

            Rectangle {
                width: 32; height: 32; radius: 7
                color: "#7c2be8"
                Label { anchors.centerIn: parent; text: "TB"; color: "white"; font.bold: true; font.pixelSize: 13 }
            }
            Label { text: "TBRetoch"; color: "white"; font.bold: true; font.pixelSize: 14 }
            Button { text: "+ Thêm ảnh"; onClicked: openDialog.open() }
            Button { text: "Copy"; enabled: appController.currentIndex >= 0; onClicked: appController.copySettings() }
            Button { text: "Paste"; enabled: appController.currentIndex >= 0; onClicked: appController.pasteSettings() }
            Button { text: "Reset"; enabled: appController.currentIndex >= 0; onClicked: appController.resetCurrentSettings() }
            Item { Layout.fillWidth: true }
            Label { text: appController.statusText; color: "#d7b5ff"; font.pixelSize: 10 }
            BusyIndicator { running: appController.busy; visible: running; implicitWidth: 25; implicitHeight: 25 }
            Button {
                text: "Xuất ảnh"
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
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#0a090c"

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    height: 38
                    color: "#131116"
                    border.color: "#26222b"
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        Label {
                            text: appController.currentName.length ? appController.currentName : "Chưa chọn ảnh"
                            color: "#d2ccd8"
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                        Button {
                            text: "After"
                            checkable: true
                            checked: viewMode === "after"
                            onClicked: viewMode = "after"
                        }
                        Button {
                            text: "Before"
                            checkable: true
                            checked: viewMode === "before"
                            onClicked: viewMode = "before"
                        }
                        Button {
                            text: "A/B"
                            checkable: true
                            checked: viewMode === "split"
                            onClicked: viewMode = "split"
                        }
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
                            id: afterImage
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
                            width: 70; height: 70; radius: 18; color: "#7428d8"
                            anchors.horizontalCenter: parent.horizontalCenter
                            Label { anchors.centerIn: parent; text: "TB"; color: "white"; font.pixelSize: 24; font.bold: true }
                        }
                        Label { text: "TBRetoch"; color: "white"; font.pixelSize: 22; font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
                        Label { text: "Native GPU workspace • Offline"; color: "#918b99"; anchors.horizontalCenter: parent.horizontalCenter }
                        Button { text: "Nhập ảnh"; anchors.horizontalCenter: parent.horizontalCenter; onClicked: openDialog.open() }
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
                            startMouseX = mouse.x; startMouseY = mouse.y
                            startPanX = panX; startPanY = panY
                        }
                        onPositionChanged: function(mouse) {
                            if (!pressed) return
                            panX = startPanX + (mouse.x - startMouseX)
                            panY = startPanY + (mouse.y - startMouseY)
                        }
                        onDoubleClicked: resetView()
                        onWheel: function(wheel) {
                            var factor = wheel.angleDelta.y > 0 ? 1.12 : 0.89
                            zoom = Math.max(0.05, Math.min(6.0, zoom * factor))
                            wheel.accepted = true
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 124
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
                            Label { text: appController.images.length + " ảnh"; color: "#aaa3b0"; font.pixelSize: 10 }
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
                                width: 112; height: 82; radius: 4
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
            Layout.preferredWidth: 365
            Layout.fillHeight: true
            color: "#17151a"
            border.color: "#2e2933"

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                TabBar {
                    id: tabs
                    Layout.fillWidth: true
                    TabButton { text: "Màu" }
                    TabButton { text: "Chân dung" }
                    TabButton { text: "Phông nền" }
                    TabButton { text: "Trang phục" }
                    TabButton { text: "Cắt" }
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
                                Layout.fillWidth: true; height: 76; color: "#121015"; border.color: "#2d2832"
                                Column { anchors.centerIn: parent; spacing: 4
                                    Label { text: "GPU COLOR PREVIEW"; color: "#b36cff"; font.pixelSize: 9; font.bold: true }
                                    Label { text: "Preview bằng shader • Export từ ảnh gốc"; color: "#8f8996"; font.pixelSize: 10 }
                                }
                            }
                            GroupBox {
                                title: "Điều chỉnh cơ bản"
                                Layout.fillWidth: true
                                ColumnLayout {
                                    width: parent.width
                                    EditSlider { title: "Exposure"; keyName: "exposure"; from: -3; to: 3; stepSize: 0.01; Layout.fillWidth: true }
                                    EditSlider { title: "Contrast"; keyName: "contrast"; Layout.fillWidth: true }
                                    EditSlider { title: "Highlights"; keyName: "highlights"; Layout.fillWidth: true }
                                    EditSlider { title: "Shadows"; keyName: "shadows"; Layout.fillWidth: true }
                                    EditSlider { title: "Whites"; keyName: "whites"; Layout.fillWidth: true }
                                    EditSlider { title: "Blacks"; keyName: "blacks"; Layout.fillWidth: true }
                                    EditSlider { title: "Temperature"; keyName: "temperature"; Layout.fillWidth: true }
                                    EditSlider { title: "Tint"; keyName: "tint"; Layout.fillWidth: true }
                                    EditSlider { title: "Vibrance"; keyName: "vibrance"; Layout.fillWidth: true }
                                    EditSlider { title: "Saturation"; keyName: "saturation"; Layout.fillWidth: true }
                                }
                            }
                            GroupBox {
                                title: "Xuất ảnh"
                                Layout.fillWidth: true
                                ColumnLayout {
                                    width: parent.width
                                    RowLayout {
                                        Label { text: "Định dạng"; color: "#ddd8e3"; Layout.fillWidth: true }
                                        ComboBox { id: formatBox; model: ["jpg", "png", "webp"]; currentIndex: 0 }
                                    }
                                    RowLayout {
                                        Label { text: "JPEG/WebP quality"; color: "#ddd8e3"; Layout.fillWidth: true }
                                        Label { text: Math.round(qualitySlider.value); color: "#c994ff" }
                                    }
                                    Slider { id: qualitySlider; from: 70; to: 100; value: 98; stepSize: 1; Layout.fillWidth: true }
                                }
                            }
                        }
                    }

                    Rectangle {
                        color: "transparent"
                        Label { anchors.centerIn: parent; width: parent.width - 40; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; color: "#aaa3b0"; text: "Portrait semantic engine đang được chuyển sang landmark/mask worker riêng. Không phát hành slider giả." }
                    }
                    Rectangle {
                        color: "transparent"
                        Label { anchors.centerIn: parent; width: parent.width - 40; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; color: "#aaa3b0"; text: "Background engine sẽ dùng matte/depth/object masks độc lập, không dùng vùng ước lượng." }
                    }
                    Rectangle {
                        color: "transparent"
                        Label { anchors.centerIn: parent; width: parent.width - 40; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; color: "#aaa3b0"; text: "Clothing engine sẽ có wrinkle/blemish/lint/extraction/color khi semantic clothing mask hoàn tất." }
                    }
                    Rectangle {
                        color: "transparent"
                        Label { anchors.centerIn: parent; color: "#aaa3b0"; text: "Crop / Rotate / Perspective worker" }
                    }
                }
            }
        }
    }

    Rectangle {
        visible: toastText.length > 0
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 145
        width: Math.min(720, toastLabel.implicitWidth + 36)
        height: toastLabel.implicitHeight + 22
        radius: 8
        color: "#2a2031"
        border.color: "#75469a"
        z: 100
        Label { id: toastLabel; anchors.centerIn: parent; text: toastText; color: "white"; wrapMode: Text.Wrap; horizontalAlignment: Text.AlignHCenter }
    }
}
