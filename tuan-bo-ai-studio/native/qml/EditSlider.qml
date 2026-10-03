import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    enabled: appController.currentIndex >= 0
    property string title: ""
    property string keyName: ""
    property real from: -100
    property real to: 100
    property real stepSize: 1
    implicitHeight: 50

    ColumnLayout {
        anchors.fill: parent
        spacing: 3

        RowLayout {
            Layout.fillWidth: true
            Label { text: root.title; color: "#e9e5ee"; font.pixelSize: 11; Layout.fillWidth: true }
            Label { text: Number(slider.value).toFixed(root.stepSize < 1 ? 2 : 0); color: "#c994ff"; font.pixelSize: 10 }
        }

        Slider {
            id: slider
            Layout.fillWidth: true
            from: root.from
            to: root.to
            stepSize: root.stepSize

            onPressedChanged: {
                if (pressed) appController.beginSettingEdit()
                else appController.endSettingEdit()
            }
            onMoved: {
                appController.setSetting(root.keyName, value)
                if (!pressed) appController.endSettingEdit()
            }

            Binding {
                target: slider
                property: "value"
                value: Number(appController.currentSettings[root.keyName] ?? 0)
                when: !slider.pressed
                restoreMode: Binding.RestoreBinding
            }
        }
    }
}
