import QtQuick 2.0

Item {
	id: wrapper
	property alias contentWidth: view.contentWidth
	readonly property alias contentHeight: view.contentHeight
	property alias model: view.model
	property alias delegate: view.delegate
	property alias contentX: flick.contentX
	property alias contentY: flick.contentY
	property alias headerItem: headeritem.item
	property alias highlight: view.highlight
	property alias currentIndex: view.currentIndex
	property alias visibleArea: flick.visibleArea
	property Component header: Component {Item {width: view.width}}
	Flickable {
		id: headerflick
		anchors { left: parent.left; right: parent.right }
		height: headeritem.height
		contentWidth: flick.contentWidth
		flickableDirection: Flickable.HorizontalFlick
		onContentXChanged: if (movingHorizontally) flick.contentX = contentX
		clip: true
		Loader {
			id: headeritem
			sourceComponent: header
			onWidthChanged: {
				headerflick.contentX = flick.contentX
			}
		}
	}
	Flickable {
		id: flick
		anchors { top: parent.top; topMargin: headerItem.height; bottom: parent.bottom; left: parent.left; right: parent.right }
		contentWidth: proxy.width; contentHeight: proxy.height
		Item { id: proxy; height: contentHeight; width: contentWidth }
		onContentYChanged: { view.y = contentY<0 ? -contentY : contentY }
		onContentXChanged: { if (movingHorizontally) {headerflick.contentX = contentX} view.x = contentX }
		ListView {
			id: view
			clip:true
			width: flick.width;	height: flick.height
			contentX: flick.contentX; contentY: flick.contentY
			interactive: false
		}
	}
}
