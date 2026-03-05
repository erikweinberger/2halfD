#include "math_util.h"
#include "TwoHalfD/engine_types.h"

std::vector<point2d> findCircleLineSegmentIntercept(const float cx, const float cy, const float r, const point2d &wallS, const point2d &wallE) {
    const float xDir = wallS[0] - wallE[0];
    std::vector<point2d> result;

    if (std::abs(xDir) < std::numeric_limits<float>::epsilon()) {
        float xDist = std::abs(wallS[0] - cx);
        if (xDist > r) return result;

        float yd = std::sqrt(r * r - xDist * xDist);
        float y1 = cy + yd;
        float y2 = cy - yd;

        const float yMax = std::max(wallS[1], wallE[1]);
        const float yMin = std::min(wallS[1], wallE[1]);
        if (y1 >= yMin && y1 <= yMax) {
            result.push_back({wallS[0], y1});
        }
        if (y2 >= yMin && y2 <= yMax) {
            result.push_back({wallS[0], y2});
        }
    } else {

        float m = (wallS[1] - wallE[1]) / (wallS[0] - wallE[0]);
        const float c = wallS[1] - m * wallS[0];

        const float mS = m * m;
        const float B = (2 * (m * c - m * cy - cx));
        const float A = mS + 1;
        const float C = (cy - c) * (cy - c) + cx * cx - r * r;
        const float disc = B * B - 4 * A * C;

        if (disc < 0) return result;

        const float x1 = (-B + std::sqrt(disc)) / (2 * A);
        const float x2 = (-B - std::sqrt(disc)) / (2 * A);

        const float lambda1 = (x1 - wallS[0]) / (wallE[0] - wallS[0]);
        const float lambda2 = (x2 - wallS[0]) / (wallE[0] - wallS[0]);

        if (lambda1 >= 0.0f && lambda1 <= 1.0f) {
            result.push_back({x1, m * x1 + c});
        }
        if (lambda2 >= 0.0f && lambda2 <= 1.0f) {
            result.push_back({x2, m * x2 + c});
        }
    }
    return result;
}

TwoHalfD::XYVectorf computeLineIntersection(const TwoHalfD::XYVectorf &p1, const TwoHalfD::XYVectorf &p2, const TwoHalfD::XYVectorf &p3,
                                            const TwoHalfD::XYVectorf &p4) {
    float A1 = p2.y - p1.y;
    float B1 = p1.x - p2.x;
    float C1 = A1 * p1.x + B1 * p1.y;

    float A2 = p4.y - p3.y;
    float B2 = p3.x - p4.x;
    float C2 = A2 * p3.x + B2 * p3.y;

    float determinant = A1 * B2 - A2 * B1;
    std::cout << "Computing line intersection, determinant: " << determinant << std::endl;

    if (std::abs(determinant) < 1e-5f) {
        return {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN()};
    } else {
        float x = (B2 * C1 - B1 * C2) / determinant;
        float y = (A1 * C2 - A2 * C1) / determinant;
        return {x, y};
    }
}
