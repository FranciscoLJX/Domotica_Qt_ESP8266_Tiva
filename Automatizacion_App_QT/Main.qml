import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    visible: true
    width: 480
    height: 800
    title: qsTr("Control Domótico")

    StackView {
        id: stack
        anchors.fill: parent

        initialItem: Item {
            Rectangle {
                anchors.fill: parent
                color: "#fef9fc"

                ColumnLayout {
                   anchors.horizontalCenter: parent.horizontalCenter
                   anchors.verticalCenter: parent.verticalCenter
                   anchors.verticalCenterOffset: -200 
                   spacing: 30

                    Text {
                        text: "Selecciona una opción"
                        font.pointSize: 26
                        font.bold: true
                        color: "#333"
                        horizontalAlignment: Text.AlignHCenter
                        Layout.alignment: Qt.AlignHCenter
                    }

                    // Botones de habitaciones
                    Button {
                        text: "Habitación 1"
                        width: 220
                        Layout.alignment: Qt.AlignHCenter
                        onClicked: {
                            stack.push(Qt.resolvedUrl("Hab1Controls.qml"), { roomId: 1, StackView: stack })
                        }
                    }

                    Button {
                        text: "Habitación 2"
                        width: 220
                        Layout.alignment: Qt.AlignHCenter
                        onClicked: {
                            stack.push(Qt.resolvedUrl("Hab2Controls.qml"), { roomId: 2, StackView: stack })
                        }
                    }

                    Button {
                        text: "Todas la habitaciones"
                        width: 220
                        Layout.alignment: Qt.AlignHCenter
                        onClicked: {
                            // prepara la lista de peers (una sola vez al arrancar también vale)
                            myController.addPeer("10.216.162.171", 8080)
                            myController.addPeer("10.216.162.154", 8080)
                            // si, más habitaciones, añádelas aquí
                            stack.push("GlobalControls.qml")
                        }
                    }

                }
            }
        }
    }
}
