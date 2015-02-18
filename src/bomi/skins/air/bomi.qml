import QtQuick 2.0
import QtQuick.Controls.Styles 1.0
import bomi 1.0 as B

B.BaseApp {
    id: app
    name: "net.xylosper.air"

    states: State {
        name: "hidden"; when: !B.App.window.mouse.cursor
        PropertyChanges { target: textBox; opacity: 0.0 }
        PropertyChanges {
            target: bottomControls; anchors.bottomMargin: -bottomControls.height
        }
    }

    transitions: Transition {
        reversible: true; to: "hidden"
        ParallelAnimation {
            SequentialAnimation {
                NumberAnimation { target: textBox; property: "opacity"; duration: 200 }
    //            PropertyAction { property: "visible" }
            }
            SequentialAnimation {
                NumberAnimation { target: bottomControls; property: "anchors.bottomMargin"; duration: 200 }
    //            PropertyAction { property: "visible" }
            }
        }
    }

    Item {
        id: textBox
        anchors { fill: parent; margins: 20 }
        opacity: 0.8

        property real fh: Math.min(app.height * 0.04, 30)

        B.TextStyle {
            id: ts
            color: "white"
            style: Text.Outline
            styleColor: "black"
            monospace: false
            font.pixelSize: textBox.fh * 0.95
        }

        B.Text {
            id: media
            width: parent.width
            height: contentHeight
            textStyle: ts
            elide: Text.ElideRight
            textFormat: Text.StyledText
            text: "[%1] <b>".arg(formatFraction(B.App.playlist.loaded + 1,
                                                B.App.playlist.count))
                  + B.App.engine.media.name + "</b>"
            font.pixelSize: textBox.fh
        }

        Row {
            anchors { top: media.bottom; topMargin: 3; left: parent.left }
            B.TimeDuration { width: implicitWidth; msec: false; textStyle: ts }
            B.Text { textStyle: ts; text: " (%1%)".arg((B.App.engine.rate * 100).toFixed(1)) }
        }
    }


    Item {
        id: bottomControls
        width: parent.width
        height: 40
        anchors.bottom: parent.bottom
        B.TimeSlider {
            id: timeSlider
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }

            height: 5
            style: SliderStyle {
                groove: Item {
                    opacity: 0.7
                    height: childrenRect.height
                    Column {
                        width: control.rate * parent.width
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: "indianred"
                        }
                        Rectangle {
                            width: parent.width
                            height: 3
                            color: "#a00"
                        }
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: "darkred"
                        }
                    }

                    Column {
                        width: (1 - control.rate) * parent.width
                        anchors.right: parent.right
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: "lightgray"
                        }
                        Rectangle {
                            width: parent.width
                            height: 3
                            color: "gray"
                        }
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: "#666"
                        }
                    }
                }
                handle: Item {

                }
            }
        }
    }

}

