import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    objectName: "edit-" + keyName
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

    TextField {
        id: valueLabel
        objectName: "value-" + root.keyName
        anchors.top: parent.top
        anchors.right: parent.right
        width: 62
        height: 20
        padding: 1
        background: Rectangle { color: "#231c2b"; radius: 3 }
        validator: DoubleValidator { bottom: root.from; top: root.to; locale: "en_US" }
        onEditingFinished: {
            if (acceptableInput && isFinite(Number(text))) { appController.setSetting(root.keyName,Number(text)); appController.endSettingEdit() }
            focus = false
        }
        text: Number(slider.value).toFixed(root.stepSize < 1 ? 2 : 0)
        color: "#c994ff"
        font.pixelSize: 10
        horizontalAlignment: Text.AlignRight
    }

    Slider {
        id: slider
        objectName: "slider-" + root.keyName
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
