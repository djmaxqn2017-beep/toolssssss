import QtQuick
import QtQuick.Controls

GroupBox {
    id: section
    property bool expanded: true
    topPadding: 34
    bottomPadding: expanded ? 10 : 0
    implicitHeight: expanded ? implicitContentHeight + topPadding + bottomPadding : topPadding
    label: Item {
        width: section.width - 24
        height: 30
        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 7
            Label { text: section.expanded ? "▾" : "▸"; color: "#a273de" }
            Label { text: section.title; color: "#ede8f2"; font.bold: true; font.pixelSize: 12 }
        }
        MouseArea { anchors.fill: parent; onClicked: section.expanded = !section.expanded; cursorShape: Qt.PointingHandCursor }
    }
    Binding { target: section.contentItem; property: "visible"; value: section.expanded }
}
