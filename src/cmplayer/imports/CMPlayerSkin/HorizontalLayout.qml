import QtQuick 2.0

Item {
	id: root
	property real spacing: 0
	property real topPadding: paddings
	property real bottomPadding: paddings
	property real leftPadding: paddings
	property real rightPadding: paddings
	property real paddings: 0
	property real contentHeight: height - topPadding - bottomPadding
	property var fillers: []
	onLeftPaddingChanged: layout()
	onRightPaddingChanged: layout()
	onPaddingsChanged: layout()
	function layout() {
		if (children.length === 0)
			return
		var w = 0, h = height - topPadding - bottomPadding
		for (var i=0; i < children.length; ++i) {
			var child = children[i];
			child.height = h
			if (fillers.indexOf(child) === -1)
				w += child.width
		}
		if (fillers.length > 0) {
			w += spacing*(children.length-1)
			w = (width - w - leftPadding - rightPadding)/fillers.length
			for (var i=0; i<fillers.length; ++i)
				fillers[i].width = w
		}
		var x = leftPadding
		for (var i=0; i<children.length; ++i) {
			var child = children[i]
			child.x = x
			child.y = topPadding
			x += child.width + spacing
		}
	}
	onHeightChanged: layout();
	onWidthChanged: layout()
	onChildrenChanged: layout()
	onSpacingChanged: layout()
	onVisibleChanged: layout()
	Component.onCompleted: {
		layout();
		for (var i=0; i<children.length; ++i) {
			var child = children[i]
			if (fillers.indexOf(child) === -1)
				child.widthChanged.connect(layout)
		}
	}
	Component.onDestruction: {
		for (var i=0; i<children.length; ++i) {
			var child = children[i]
			if (fillers.indexOf(child) === -1)
				child.widthChanged.disconnect(layout)
		}
	}
}
