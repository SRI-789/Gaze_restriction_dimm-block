pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtWebEngine
import QtWebChannel

ApplicationWindow {
    id: window
    width: 900
    height: 650
    minimumWidth: 760
    minimumHeight: 560
    visible: true
    title: "Eye Attention — Qt"
    color: "#f3f6fa"

    WebChannel {
        id: trackerChannel
        registeredObjects: [attentionController]
    }

    WebEngineView {
        id: tracker
        x: 0
        y: 0
        width: 2
        height: 2
        opacity: 0.01
        z: 0.5
        url: "qrc:/tracker.html"
        webChannel: trackerChannel
        settings.localContentCanAccessRemoteUrls: true

        onFeaturePermissionRequested: function(origin, feature) {
            if (feature === WebEngineView.MediaVideoCapture) {
                grantFeaturePermission(origin, feature, true)
            } else {
                grantFeaturePermission(origin, feature, false)
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#f3f6fa"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 28
        spacing: 18
        z: 1

        Label {
            text: "Eye Attention"
            font.pixelSize: 28
            font.bold: true
            color: "#18212f"
        }

        Label {
            Layout.fillWidth: true
            text: "Local gaze estimate. Looking outside the calibrated screen zone dims after 3 seconds and restricts the desktop after 10 seconds."
            wrapMode: Text.WordWrap
            color: "#526176"
            font.pixelSize: 15
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Button {
                text: attentionController.running ? "Camera running" : "Start camera"
                enabled: !attentionController.running
                onClicked: attentionController.start()
            }

            Button {
                text: attentionController.calibrating ? "Calibrating..." : "Calibrate screen center"
                enabled: attentionController.running && !attentionController.calibrating
                onClicked: attentionController.calibrateScreen()
            }

            Button {
                text: "Stop"
                enabled: attentionController.running
                onClicked: attentionController.stop()
            }

            Button {
                text: "Reset totals"
                onClicked: attentionController.resetTotals()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 16
            color: "white"
            border.color: "#dce3ec"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 16

                Label {
                    text: "Status"
                    font.pixelSize: 14
                    color: "#58677b"
                }

                Label {
                    Layout.fillWidth: true
                    text: attentionController.statusText
                    wrapMode: Text.WordWrap
                    font.pixelSize: 19
                    font.bold: true
                    color: "#18212f"
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    rowSpacing: 18
                    columnSpacing: 24

                    Repeater {
                        model: [
                            { label: "Current away glance", value: attentionController.currentAwayText },
                            { label: "Looking at screen", value: attentionController.screenTimeText },
                            { label: "Looking away", value: attentionController.awayTimeText },
                            { label: "Dimming / restricted", value: "3 s / 10 s" }
                        ]

                        delegate: RowLayout {
                            required property var modelData

                            Layout.fillWidth: true
                            Layout.columnSpan: 2
                            spacing: 24

                            Label {
                                Layout.fillWidth: true
                                text: modelData.label
                                color: "#58677b"
                            }

                            Label {
                                text: modelData.value
                                font.bold: true
                                font.pixelSize: 20
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                Label {
                    Layout.fillWidth: true
                    text: "Approximate webcam estimate — not a secure Windows lock. Use Win+L for actual access protection."
                    wrapMode: Text.WordWrap
                    color: "#526176"
                }
            }
        }
    }

    Window {
        id: privacyOverlay
        flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool | Qt.WindowTransparentForInput
        color: "transparent"
        visible: attentionController.privacyState !== "hidden"
        x: Qt.application.screens.length ? Qt.application.screens[0].virtualGeometry.x : 0
        y: Qt.application.screens.length ? Qt.application.screens[0].virtualGeometry.y : 0
        width: Qt.application.screens.length ? Qt.application.screens[0].virtualGeometry.width : 0
        height: Qt.application.screens.length ? Qt.application.screens[0].virtualGeometry.height : 0

        Rectangle {
            anchors.fill: parent
            color: attentionController.privacyState === "restricted"
                ? "#000000"
                : "#66000000"

            Text {
                anchors.centerIn: parent
                text: attentionController.privacyState === "restricted"
                    ? "Restricted for others"
                    : "Developer deviation"
                color: attentionController.privacyState === "restricted" ? "white" : "red"
                font.pixelSize: Math.max(36, Math.min(parent.width, parent.height) * 0.07)
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
