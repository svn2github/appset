import QtQuick 1.0
import QtWebKit 1.0

Rectangle {
    id: page
    width: 1050; height: 400
    gradient: Gradient {
             GradientStop { position: 0.0; color: "#8C8F8C" }
             GradientStop { position: 0.17; color: "#6A6D6A" }
             GradientStop { position: 0.98;color: "#3F3F3F" }
             GradientStop { position: 1.0; color: "#0e1B20" }
         }

    Image {
        anchors { right: page.right; top: page.top }
        source: "moreUp.png"
        opacity: listView.atYBeginning || page.state=="Details"? 0 : 1
        z:2
        MouseArea {
            anchors.fill: parent
            onClicked: listView.contentY-=listView.contentY>=0?52:0;
        }
    }

    Image {
        anchors { right: page.right; bottom: page.bottom }
        source: "moreDown.png"
        opacity: listView.atYEnd || page.state=="Details" ? 0 : 1
        z:2
        MouseArea {
            anchors.fill: parent
            onClicked: listView.contentY+=52;
        }
    }


    Component {
        id: appDelegate

        Item {
            id: app

            property real detailsOpacity : 0
            property string s: status
            property string preS: status

            width: listView.cellWidth
            height: listView.cellHeight
            z: 0

            Rectangle {
                id: background
                x: 2; y: 2; width: parent.width - x*2; height: parent.height - y*2
                color: "black"
                gradient: Gradient {
                    GradientStop {
                        id:gr1;
                        position: 0.00;
                        color: "#6a6d6a";
                    }
                    GradientStop {
                        id:gr2;
                        position: 0.50;
                        color: "#3f3f3f";
                    }
                    GradientStop {
                        position: 1.00;
                        color: "#0e1b20";
                    }
                }
                radius: 5
                border.color: "black"
            }

            function filterInstalled(){
                if(app.s=="Installed" || app.s=="Remove") return 0;
                return 1;
            }

            function sizeFormat(){
                //dsize from KB
                if(dsize>=1024*1024) return "~"+Math.round(dsize/(1024.*1024))+" GB"
                if(dsize>=1024) return "~"+Math.round(dsize/1024.)+" MB"
                return dsize+" KB"
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    app.state = page.state = 'Details'
                    listView.currentIndex=index
                    if(app.s=="Remote" || app.s=="Install"){
                        lversionid.opacity=lversionstrid.opacity=0
                    }
                }
                hoverEnabled: true
                onHoveredChanged: {
                    if(app.state=="Details")return;
                    if(containsMouse)
                        background.border.color="darkgrey"
                    else
                        background.border.color="black"
                }
                onPressed: {
                    if(app.state=="Details")return;
                    gr1.position=0.5
                    gr2.position=0
                }
                onExited: {
                    gr1.position=0
                    gr2.position=0.5
                }
                onReleased: {
                    gr1.position=0
                    gr2.position=0.5
                }
            }

            Row {
                id: topLayout
                x: 10; y: 10; height: appImage.height; width: parent.width
                spacing: 6

                Image {
                    id: appImage
                    width: 33; height: 33
                    source: s
                }

                Column {
                    width: background.width - appImage.width - 20; height: appImage.height
                    spacing: 2

                    Text {
                        text: name
                        wrapMode: Text.WordWrap
                        elide:"ElideRight"
                        font.bold: true; font.pointSize: 11;
                        color: "white"
                        width: parent.width-5
                        id: appName
                    }

                    Text { id: methodText; text: description; wrapMode: Text.WordWrap; width: parent.width-2;
                        anchors.top: name.bottom;elide:"ElideRight";color: "lightgrey" }

                    Row{
                        spacing: 10
                        id: extraInfo
                        opacity: app.detailsOpacity

                        Text {
                            text:  repostr+":"
                            font.pointSize: 9; font.bold: true
                            opacity: app.detailsOpacity && filterInstalled()
                        }

                        Text {
                            text: repo
                            font.pointSize: 9
                            opacity: app.detailsOpacity && filterInstalled()
                        }

                        Text {
                            text: lversionstr+":"
                            font.pointSize: 9; font.bold: true                            
                            id: lversionstrid
                        }

                        Text {
                            text: lversion
                            font.pointSize: 9;
                            id: lversionid
                        }

                        Text {
                            text: rversionstr+":"
                            font.pointSize: 9; font.bold: true
                        }

                        Text {
                            text: rversion
                            font.pointSize: 9;
                        }

                        Text {
                            text: "URL:"
                            font.pointSize: 9; font.bold: true
                        }

                        Text {
                            text: appUrl
                            font.italic: true
                            font.pointSize: 9
                            color: "blue"
                            font.underline: true;

                            MouseArea{
                                anchors.fill: parent;
                                onClicked: appset.extBrowserLink(appUrl);
                            }

                        }
                    }                    

                    Row{
                        spacing: 3;
                        opacity: app.detailsOpacity && filterInstalled()                        

                        Text {
                            text:  dsizestr+":"
                            font.pointSize: 9; font.bold: true
                        }

                        Text {
                            text: sizeFormat()
                            font.pointSize: 9
                        }
                    }

                    Row{
                        spacing: 3;
                        TextButton {
                            opacity: app.detailsOpacity && (s=="Remote")
                            text: qsTr("Install")
                            onClicked: {
                                appset.setCurrentRow(i);
                                appset.install();
                                app.preS=app.s
                                app.s="Install"
                            }
                        }
                        TextButton {
                            opacity: app.detailsOpacity && (s=="Upgradable" || s=="Installed")
                            text: qsTr("Remove")
                            onClicked: {
                                appset.setCurrentRow(i);
                                appset.remove();
                                app.preS=app.s
                                app.s="Remove"
                            }
                        }
                        TextButton {
                            opacity: app.detailsOpacity && (s=="Upgradable")
                            text: qsTr("Upgrade")
                            onClicked: {
                                appset.setCurrentRow(i);
                                appset.upgrade();
                                app.preS=app.s
                                app.s="Upgrade"
                            }
                        }
                        TextButton {
                            opacity: app.detailsOpacity && (s=="Install" || s=="Remove" || s=="Upgrade")
                            text: qsTr("Cancel")
                            onClicked: {
                                appset.setCurrentRow(i);
                                if(app.s=="Upgrade")appset.notUpgrade()
                                else if(app.s=="Remove")appset.notRemove()
                                else if(app.s=="Install")appset.notInstall()
                                app.s=app.preS
                            }
                        }
                    }

                }
            }            

            Item {
                id: details
                x: 10; width: parent.width - 20
                anchors { top: topLayout.bottom; topMargin: 10; bottom: parent.bottom; bottomMargin: 10 }
                opacity: app.detailsOpacity


                Flickable {
                    id: flick
                    width: parent.width
                    anchors { top: details.top; bottom: parent.bottom }
                    contentHeight: appWeb.height
                    clip: true


                    WebView{
                        url: "about:blank"
                        height: 2000
                        width: parent.width-20
                        x:10
                        id: appWeb
                    }
                }

                Image {
                    anchors { right: flick.right; top: flick.top }
                    source: "moreUp.png"
                    opacity: flick.atYBeginning ? 0 : 1
                    MouseArea {
                        anchors.fill: parent
                        onClicked: flick.contentY-=flick.contentY>=0?52:0;
                    }
                }

                Image {
                    anchors { right: flick.right; bottom: flick.bottom }
                    source: "moreDown.png"
                    opacity: flick.atYEnd ? 0 : 1
                    MouseArea {
                        anchors.fill: parent
                        onClicked: flick.contentY+=52;
                    }
                }                
            }

            TextButton {
                y: 10
                anchors { right: background.right; rightMargin: 10 }
                opacity: app.detailsOpacity
                text: qsTr("Close")

                onClicked: app.state = page.state = '';
            }

            states: [
                State {
                    name: "Details"
                    PropertyChanges { target: appImage; width: 96; height: 96 } // Make picture bigger
                    PropertyChanges { target: appName; font.pointSize: 16 }
                    PropertyChanges { target: app; detailsOpacity: 1; x: 0; z:10 } // Make details visible
                    PropertyChanges { target: app; height: listView.height } // Fill the entire list area with the detailed view
                    PropertyChanges { target: app; width: listView.width} // Fill the entire list area with the detailed view
                    PropertyChanges { target: appWeb; width: listView.width-20} // Fill the entire list area with the detailed view
                    PropertyChanges { target: app.GridView.view; explicit: true; contentY: app.y}
                    PropertyChanges { target: app.GridView.view; interactive: false }
                    PropertyChanges {
                        target: appWeb
                        url: appUrl
                    }
                    PropertyChanges {
                        target: appWeb
                        height: appWeb.contentsSize.height
                    }
                }
            ]

            transitions: Transition { NumberAnimation { duration: 180; properties: "detailsOpacity,x,z,contentY,height,width,font.pointSize" } }
        }
    }

    function closeDetails(){
        listView.currentItem.state=""
    }

    GridView {
        id: listView
        anchors.fill: parent
        model: appsModel
        delegate: appDelegate
        cellWidth: 206
        cellHeight: 52
    }

}
