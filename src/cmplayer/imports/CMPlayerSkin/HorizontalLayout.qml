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
	property bool centering: false
	property var fillers: []
	property bool __doing: false
	function layout() {
		if (children.length === 0)
			return
		var w = 0, hMax = height - topPadding - bottomPadding
		for (var i=0; i < children.length; ++i) {
			var child = children[i];
			child.height = hMax
			child.y = topPadding
			if (fillers.indexOf(child) === -1)
				w += child.width
		}
		if (fillers.length > 0) {
			w += spacing*(children.length-1)
			w = (width - w - leftPadding - rightPadding)/fillers.length
			for (i=0; i<fillers.length; ++i)
				fillers[i].width = w
		}
		var x = leftPadding
		for (i=0; i<children.length; ++i) {
			child = children[i]
			child.x = x
			x += child.width + spacing
		}
	}
	function center() {
		if (centering) {
			for (var i=0; i<children.length; ++i) {
				children[i].anchors.verticalCenter = verticalCenter
			}
		}
	}
	onHeightChanged: center()
	onWidthChanged: layout()
	Component.onCompleted: {
		layout(); center();
		childrenChanged.connect(center)
		childrenChanged.connect(layout)
		for (var i=0; i<children.length; ++i) {
			var child = children[i]
			if (fillers.indexOf(child) === -1)
				child.widthChanged.connect(layout)
		}
	}
}
