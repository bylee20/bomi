import QtQuick 2.0

Item {
    property Item source: Item { }
    property size textureSize: source.textureSize
        ? source.textureSize : Qt.size(source.width, source.height)

    readonly property string __blurVtx: "
        uniform highp mat4 qt_Matrix;
        attribute highp vec4 qt_Vertex;
        attribute highp vec2 qt_MultiTexCoord0;
        varying highp vec2 coord;
        void main() {
            coord = qt_MultiTexCoord0;
            gl_Position = qt_Matrix * qt_Vertex;
        }"

    ShaderEffect {
        id: pass1
        readonly property Item src: source
        property real dx: 1.0 / textureSize.width
        anchors.fill: parent; visible: false
        vertexShader: __blurVtx
        fragmentShader: "
            varying highp vec2 coord;
            uniform sampler2D src;
            uniform lowp float qt_Opacity;
            uniform lowp float dx;
            void main() {
                lowp vec4 tex = vec4(0.0, 0.0, 0.0, 0.0);
                tex += texture2D(src, coord + vec2(-2.0*dx, 0.0)) * 0.064;
                tex += texture2D(src, coord + vec2(-dx, 0.0)) * 0.244;
                tex += texture2D(src, coord) * 0.383;
                tex += texture2D(src, coord + vec2(dx, 0.0)) * 0.244;
                tex += texture2D(src, coord + vec2(2.0*dx, 0.0)) * 0.064;
                gl_FragColor = tex;

            }"
    }

    ShaderEffectSource {
        id: interm
        sourceItem: pass1; visible: false
        anchors.fill: parent
        textureSize: parent.textureSize
        wrapMode: ShaderEffectSource.ClampToEdge
    }

    ShaderEffect {
        id: pass2
        readonly property Item src: interm
        property real dy: 1.0 / textureSize.height
        anchors.fill: parent
        vertexShader: __blurVtx
        fragmentShader: "
            varying highp vec2 coord;
            uniform sampler2D src;
            uniform lowp float qt_Opacity;
            uniform lowp float dy;
            void main() {
                lowp vec4 tex = vec4(0.0, 0.0, 0.0, 0.0);
                tex += texture2D(src, coord + vec2(0.0, -2.0*dy)) * 0.064;
                tex += texture2D(src, coord + vec2(0.0, -dy)) * 0.244;
                tex += texture2D(src, coord) * 0.383;
                tex += texture2D(src, coord + vec2(0.0, dy)) * 0.244;
                tex += texture2D(src, coord + vec2(0.0, 2.0*dy)) * 0.064;
                gl_FragColor = tex;

            }"
    }
}
