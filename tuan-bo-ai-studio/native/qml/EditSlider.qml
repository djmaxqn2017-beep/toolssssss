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
    implicitHeight: 54

    Label {
        objectName: "slider-title-" + root.keyName
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.right: valueLabel.left
        anchors.rightMargin: 8
        text: root.title
        color: "#e9e5ee"
        font.pixelSize: 11
        elide: Text.ElideRight
    }

    Label {
        id: valueLabel
        anchors.top: parent.top
        anchors.right: parent.right
        width: 62
        text: Number(slider.value).toFixed(root.stepSize < 1 ? 2 : 0)
        color: "#c994ff"
        font.pixelSize: 10
        horizontalAlignment: Text.AlignRight
    }

    Slider {
        id: slider
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 20
        height: 28
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
