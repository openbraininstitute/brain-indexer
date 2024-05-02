#pragma once

#include <brain_indexer/index.hpp>
#include <zisa/math/space_filling_curve.hpp>

namespace brain_indexer {
namespace experimental {

std::vector<size_t> space_filling_order(Point3Dx const * points, size_t n_points) {
    auto sfc_index = std::vector<size_t>(n_points);

    auto float_max = std::numeric_limits<CoordType>::max();
    auto float_min = std::numeric_limits<CoordType>::lowest();

    auto min_corner = Point3Dx{float_max, float_max, float_max};
    auto max_corner = Point3Dx{float_min, float_min, float_min};

    for(size_t i = 0; i < n_points; ++i) {
        min_corner = min(min_corner, points[i]);
        max_corner = max(max_corner, points[i]);
    }

    auto box_size = max_corner - min_corner;

    auto normalize = [min_corner, box_size](const Point3D& xyz) {
        return Point3D{
            (xyz.get<0>() - min_corner.get<0>()) / box_size.get<0>(),
            (xyz.get<1>() - min_corner.get<1>()) / box_size.get<1>(),
            (xyz.get<2>() - min_corner.get<2>()) / box_size.get<2>()
        };
    };

    for(size_t i = 0; i < n_points; ++i) {
        auto xyz_norm = normalize(points[i]);
        sfc_index[i] = zisa::hilbert_index<10>(
            xyz_norm.get<0>(),
            xyz_norm.get<1>(),
            xyz_norm.get<2>()
        ).to_ulong();
    }

    auto order = std::vector<size_t>(n_points);
    for(size_t i = 0; i < n_points; ++i) {
        order[i] = i;
    }

    std::sort(order.begin(), order.end(), [&sfc_index](size_t i, size_t j) {
         return sfc_index[i] < sfc_index[j];
    });

    return order;
}

std::vector<size_t> space_filling_order(const std::vector<Point3Dx> & points) {
    return space_filling_order(points.data(), points.size());
}


}
}