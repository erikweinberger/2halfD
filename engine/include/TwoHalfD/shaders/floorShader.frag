uniform vec2 cameraPos;
uniform vec2 cameraDir;
uniform vec2 cameraPlane;
uniform float cameraHeight;
uniform float focalLength;
uniform sampler2D floorTexture;
uniform vec2 resolution;
uniform float textureSize;

void main() {
    vec2 screenPos = gl_FragCoord.xy;
    float y = screenPos.y;
    
    if (y > resolution.y / 2.0) {
        discard;
    }
    
    float pixelsFromCenterY = y - resolution.y / 2.0;
    float perpWorldDistance = (cameraHeight * focalLength) / pixelsFromCenterY;
    
    if (perpWorldDistance > 3000.0) {
        discard;
    }
    
    float cameraX = 2.0 * screenPos.x / resolution.x - 1.0;
    vec2 rayDir = cameraDir + cameraPlane * cameraX;
    float realRayDist = perpWorldDistance / dot(rayDir, cameraDir);
    
    vec2 floorPos = cameraPos + rayDir * realRayDist;
    
    vec2 texCoord = fract(floorPos / textureSize);
    vec4 color = texture2D(floorTexture, texCoord);
    
    float shade = min(1.0, 256.0 / perpWorldDistance);
    gl_FragColor = vec4(color.rgb * shade, 1.0);
}