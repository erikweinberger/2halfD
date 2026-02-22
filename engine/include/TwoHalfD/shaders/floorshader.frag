uniform vec2 textureStartCord;
uniform sampler2D texture;
uniform vec2 textureSize;
uniform vec2 cameraPos;
uniform float cameraHeight;
uniform vec2 n_plane;
uniform vec2 direction;
uniform float focalLength;
uniform vec2 resolution;
uniform float distanceCutoff;
uniform float shaderScale;

bool pointInsidePolygon(vec2 p);

void main() {
    vec2 pixelCord = gl_FragCoord.xy;
    pixelCord.y = resolution.y - pixelCord.y;

    float pixelsFromCenterY = pixelCord.y - resolution.y / 2.0;
    float pixelsFromCenterX = pixelCord.x - resolution.x / 2.0;


    float perpWorldDistance = (cameraHeight * focalLength) / pixelsFromCenterY;

    if ((perpWorldDistance < 0.001 || perpWorldDistance > distanceCutoff)) {
        discard;
        return;
    }
    
    float planeDist = perpWorldDistance * pixelsFromCenterX / focalLength;
    vec2 planeScaled = n_plane * planeDist;
    vec2 directionScaled = direction * perpWorldDistance;

    vec2 worldPos = cameraPos + planeScaled + directionScaled;

    vec2 floorPos = worldPos - textureStartCord;
    vec2 texCord = mod(floorPos, textureSize) / textureSize;

    vec4 pixel = texture2D(texture, texCord);

    float shade = min(1., shaderScale / perpWorldDistance);
    pixel.rgb *= shade;


    gl_FragColor = pixel;
    
}