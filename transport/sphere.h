#pragma once

#include <cmath>

namespace Sphere {
    double ConvertDegreesToRadians(double degrees);

    struct Point {
        double latitude = 0;
        double longitude = 0;

        static Point FromDegrees(double latitude, double longitude);
    };

    double Distance(Point lhs, Point rhs);
}
