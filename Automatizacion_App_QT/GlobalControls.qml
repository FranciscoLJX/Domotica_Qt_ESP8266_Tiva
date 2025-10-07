import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    anchors.fill: parent
    property alias titulo: titleLabel.text

    Column {
        anchors.centerIn: parent
        spacing: 12

        Label {
            id: titleLabel
            text: "Controles globales"
            font.pixelSize: 20
        }

        // ==== Botones de ejemplo ====
        Row {
            spacing: 10
            Button {
                text: "AUTO ON (todos)"
                onClicked: myController.sendToAll("AUTO_ON")
            }
            Button {
                text: "AUTO OFF (todos)"
                onClicked: myController.sendToAll("AUTO_OFF")
            }
        }

        Row {
            spacing: 10
            Button {
                text: "RELE1 ON (todos)"
                onClicked: myController.sendToAll("RELE1_ON")
            }
            Button {
                text: "RELE1 OFF (todos)"
                onClicked: myController.sendToAll("RELE1_OFF")
            }
        }

        Row {
            spacing: 10
            Button {
                text: "PEDIR DATOS (todos)"
                onClicked: myController.sendToAll("PEDIR_DATOS")
            }
        }

        Button {
            text: "⬅ Volver"
            width: parent.width
            onClicked: stack.pop()
        }


        // Log simple
        TextArea {
            id: log
            width: 320; height: 150
            readOnly: true
        }
    }

    // Muestra qué habitación respondió cada línea
    Connections {
        target: myController
        onMensajeRecibidoFrom: {
            log.append(key + ": " + linea)
        }
    }
}

