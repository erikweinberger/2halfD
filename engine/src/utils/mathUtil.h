
static const float PI = 3.1415927;

inline float degreeToRad(float degree) {
    return degree * PI / 180;
}

inline float radToDegree(float rad) {
    return 180.f / PI * rad;
}
