import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
  id: root
  anchors.fill: parent

  // ===== Config  =====
  property string ipArduino: "10.216.162.171"
  property int puertoArduino: 8080

  // --- helpers para el log ---
  function logClear() { log.text = "" }
  function logAppend(line) {
    if (!line || line.length === 0) return
    log.text += (log.text.length ? "\n" : "") + line
    Qt.callLater(function() {
      if (log.flickable) {
        log.flickable.contentY = Math.max(0, log.flickable.contentHeight - log.flickable.height)
      }
    })
  }

  // ===== Layout principal centrado y con ancho controlado =====
  ColumnLayout {
    id: main
    anchors.fill: parent
    anchors.margins: 16

    Frame {
      id: card
      Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
      Layout.fillHeight: false
      Layout.preferredWidth: Math.min(root.width - 32, 420)
      padding: 14
      background: Rectangle { radius: 10; color: "white"; border.color: "#dddddd" }

      ColumnLayout {
        id: content
        spacing: 12
        width: parent.width

        // Título centrado
        Label {
          text: "Habitación 1"
          font.pixelSize: 20
          horizontalAlignment: Text.AlignHCenter
          Layout.fillWidth: true
        }

        // Fila de información (IP / Puerto)
        RowLayout {
          spacing: 8
          Layout.fillWidth: true
          Label { text: "IP:"; font.bold: true }
          Label { text: ipArduino; Layout.fillWidth: true }
          Label { text: "Puerto:"; font.bold: true }
          Label { text: puertoArduino }
        }

        // Botones con rejilla para que queden alineados
        GridLayout {
          id: gridBtns
          columns: 2
          columnSpacing: 10
          rowSpacing: 10
          Layout.fillWidth: true

          Button {
            text: "Conectar"
            Layout.fillWidth: true
            onClicked: {
              logClear()
              logAppend("Conectando a " + ipArduino + ":" + puertoArduino + "…")
              myController.conectar(ipArduino, puertoArduino)
            }
          }
          Button {
            text: "Pedir datos"
            Layout.fillWidth: true
            onClicked: {
              logClear()
              myController.enviarComando("PEDIR_DATOS")
            }
          }
          Button {
            text: "Apagar luz 1"
            Layout.fillWidth: true
            onClicked: {
              logClear()
              myController.enviarComando("RELE1_ON")
            }
          }
          Button {
            text: "Encender luz 1"
            Layout.fillWidth: true
            onClicked: {
              logClear()
              myController.enviarComando("RELE1_OFF")
            }
          }

          Button {
            text: "Apagar luz 2"
            Layout.fillWidth: true
            onClicked: {
              logClear()
              myController.enviarComando("RELE2_ON")
            }
          }
          Button {
            text: "Encender luz 2"
            Layout.fillWidth: true
            onClicked: {
              logClear()
              myController.enviarComando("RELE2_OFF")
            }
          }


          Button {
            text: "Subir persiana"
            Layout.fillWidth: true
            onClicked: {
              logClear()
              myController.enviarComando("BLIND_STEP_UP")
            }
          }
          Button {
            text: "Bajar persiana"
            Layout.fillWidth: true
            onClicked: {
              logClear()
              myController.enviarComando("BLIND_STEP_DOWN")
            }
          }

          Button {
            text: "Modo Autotmatico ON"
            Layout.fillWidth: true
            onClicked: {
              logClear()
              myController.enviarComando("AUTO_ON")
            }
          }
          Button {
            text: "Modo Automatico OFF"
            Layout.fillWidth: true
            onClicked: {
              logClear()
              myController.enviarComando("AUTO_OFF")
            }
          }
        }

        Button {
            text: "⬅ Volver"
            width: parent.width
            onClicked: stack.pop()
        }

        // Área de log con autoscroll
        TextArea {
          id: log
          Layout.fillWidth: true
          Layout.preferredHeight: 180
          readOnly: true
          wrapMode: Text.NoWrap
          ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        }
      }
    }
  }

  // ===== Señales del controlador =====
  Connections {
    target: myController
    onMensajeRecibido: (linea) => logAppend(linea)

    onConnectedChanged: (c) => logAppend(c ? "✅ Conectado" : "⚠️ Desconectado")
  }
}

