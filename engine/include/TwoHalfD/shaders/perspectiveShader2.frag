uniform float wallLen;
uniform float startRatio;
uniform float endRatio;
uniform sampler2D texture;
uniform vec2 texSize;
uniform vec2 topLeft;
uniform vec2 bottomLeft;
uniform vec2 bottomRight;
uniform vec2 topRight;
uniform float leftDepth;
uniform float rightDepth;

void main() {
    vec2 objectCord = gl_FragCoord.xy;
    float width = topRight.x - topLeft.x;
    
    if (abs(width) < 0.001) {
        discard;
    }
    
    float n_xCord = (objectCord.x - topLeft.x) / width;
    
    float invZ = leftDepth * (1.0 - n_xCord) + rightDepth * n_xCord;
    float z = 1.0 / invZ;
    
    float worldPos_over_z = (startRatio * leftDepth) * (1.0 - n_xCord) + 
                            (endRatio * rightDepth) * n_xCord;
    float worldPos = worldPos_over_z * z;
    float texX = mod(worldPos * wallLen, texSize.x) / texSize.x;
    
    float leftHeight = bottomLeft.y - topLeft.y;
    float rightHeight = bottomRight.y - topRight.y;
    
    float topY = topLeft.y * (1.0 - n_xCord) + topRight.y * n_xCord;
    float bottomY = bottomLeft.y * (1.0 - n_xCord) + bottomRight.y * n_xCord;
    
    float screenT = (objectCord.y - topY) / (bottomY - topY);

    
    vec2 texCord = vec2(texX, screenT);
    vec4 pixel = texture2D(texture, texCord);
    gl_FragColor = pixel;
}