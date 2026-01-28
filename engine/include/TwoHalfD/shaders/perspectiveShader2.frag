uniform float wallLen;
uniform float startRatio;
uniform float endRatio;
uniform sampler2D texture;
uniform vec2 texSize;
uniform vec2 topLeft;
uniform vec2 bottomLeft;
uniform vec2 bottomRight;
uniform vec2 topRight;

void main() {
    vec2 objectCord = gl_FragCoord.xy;
    float width = topRight.x - topLeft.x;
    
    if (abs(width) < 0.001) {
        discard;
    }
    
    // n_xCord: 0 at left edge, 1 at right edge of this segment
    float n_xCord = (objectCord.x - topLeft.x) / width;
    
    // Map to world position along the wall
    float worldPos = startRatio + n_xCord * (endRatio - startRatio);
    
    // Convert world position to texture coordinate
    // Multiply by wallLen to get position in world units, then normalize by texture width
    float texX = mod(worldPos * wallLen, texSize.x) / texSize.x;
    
    float yLen = (bottomLeft.y - topLeft.y) * (1.0 - n_xCord) + (bottomRight.y - topRight.y) * n_xCord;
    
    if (abs(yLen) < 0.001) {
        discard;
    }
    
    float n_yCord = ((objectCord.y - topLeft.y) + (yLen - (bottomLeft.y - topLeft.y)) / 2.0) / yLen;
    
    vec2 texCord = vec2(texX, n_yCord);
    vec4 pixel = texture2D(texture, texCord);
    gl_FragColor = pixel;
}