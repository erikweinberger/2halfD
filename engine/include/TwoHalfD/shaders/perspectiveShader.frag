uniform sampler2D texture;
uniform vec2 topLeft;
uniform vec2 bottomLeft;
uniform vec2 bottomRight;
uniform vec2 topRight;


void main() {
    vec2 objectCord = gl_FragCoord.xy;  // Fixed: FragCoord not FragCord

    float width = topRight.x - topLeft.x;

    if (abs(width) < 0.001) {
        discard;  // Don't draw this pixel
        return;
    }
    
    float n_xCord = (objectCord.x - topLeft.x) / width;

    float yLen = (bottomLeft.y - topLeft.y) * (1.0 - n_xCord) + (bottomRight.y - topRight.y) * n_xCord;
    if (abs(yLen) < 0.001) {
        discard;
        return;
    }


    float n_yCord = ((objectCord.y - topLeft.y) + (yLen - (bottomLeft.y - topLeft.y)) / 2.) / yLen;
    
    vec2 texCord = vec2(n_xCord, n_yCord);
    
    vec4 pixel = texture2D(texture, texCord);

    
    gl_FragColor = pixel;
}