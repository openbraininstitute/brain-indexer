#pragma once

#include <vector>

#include <spatial_index/mpi_wrapper.hpp>

namespace spatial_index {


inline std::vector<size_t> histogram(const std::vector<CoordType>& bins,
                                     std::vector<CoordType> data,
                                     MPI_Comm comm) {

    auto mpi_rank = mpi::rank(comm);

    std::sort(data.begin(), data.end());

    auto counts = std::vector<size_t>(bins.size(), 0ul);
    size_t current_bin_idx = 0;
    for(auto x : data) {
        while(x >= bins[current_bin_idx]) {
            ++current_bin_idx;
        }

        ++counts[current_bin_idx];
    }

    MPI_Reduce(
        (mpi_rank == 0) ? MPI_IN_PLACE : counts.data(),
        counts.data(),
        util::safe_integer_cast<int>(bins.size()),
        mpi::datatype<size_t>(),
        MPI_SUM,
        0,
        comm
    );

    return (mpi_rank == 0) ? counts : std::vector<size_t>{};
}


std::vector<CoordType> log10_bins(CoordType log10_low,
                                  CoordType log10_high,
                                  size_t bins_per_decade) {

    auto n_decades = log10_high - log10_low;
    auto n_bins = 2 + bins_per_decade * n_decades;
    auto bin_width = CoordType(1.0) / bins_per_decade;
    auto bins = std::vector<CoordType>(n_bins);

    for(size_t i = 0; i < n_bins-1; ++i) {
        bins[i] = std::pow(
            CoordType(10.0),
            CoordType(log10_low + i * bin_width)
        );
    }
    bins[n_bins-1] = std::numeric_limits<CoordType>::max();

    return bins;
}


void segment_length_histogram(const std::string &output_dir, MPI_Comm comm = MPI_COMM_WORLD) {
    auto storage = NativeStorageT<MorphoEntry>(output_dir);
    auto top_tree = storage.load_top_tree();
    auto n_subtrees = top_tree.size();

    auto comm_size = mpi::size(comm);
    auto comm_rank = mpi::rank(comm);

    auto chunk = util::balanced_chunks(n_subtrees, comm_size, comm_rank);
    auto data = std::vector<CoordType>{};
    for(size_t i = chunk.low; i < chunk.high; ++i) {
        std::cout << "loading: " + std::to_string(i) + "\n";
        auto subtree = storage.load_subtree(i); 

        for(const auto &value : subtree) {
            data.push_back(characteristic_length(value));
        }
    }

    auto bins = log10_bins(CoordType(-8), CoordType(6), 4ul);
    auto counts = histogram(bins, std::move(data), comm);

    MPI_Barrier(comm);

    if(mpi::rank(comm) == 0) {
        std::cout << std::setprecision(4) << std::scientific;
        std::cout << "[      -inf, " << bins[0] << "): " << counts[0] << "\n";
        for(size_t i = 1; i < bins.size()-1; ++i) {
            std::cout << "[" << bins[i-1] << ", " << bins[i] << "): "  << counts[i] << "\n";
        }
        std::cout << "[" << bins[bins.size()-2] << ",        inf): " << counts.back() << "\n";
    }
}


void inspect_bad_cases() {
    auto storage = NativeStorageT<MorphoEntry>("circuit-1M.index");

    std::vector<size_t> subtree_ids{
        3601, 3681, 3683, 3605, 3691, 3585, 3687, 3693, 3599, 3689, 3695, 3715
        // 3721, 3591, 3597, 3603, 3617, 3661, 3629, 3627, 3631, 3623, 3655, 3625,
        // 3657, 3685, 3593, 3717, 3659, 3595, 3649, 3621, 3609, 3723, 3611, 3725,
        // 3607, 3697, 3665, 3707, 3701, 3699, 3711, 3709, 3705, 3703, 3619, 3663,
        // 3641, 3635, 3637, 3639, 3675, 3587, 3667, 3677, 3589, 3653, 3673, 3671,
        // 3679, 3633, 3669, 3651, 3767, 3727, 3729, 3713, 3719, 3761, 3811, 3733,
        // 3731, 3765, 3763, 3809, 3757, 3759, 3755, 3753, 3749, 3751, 3745, 3747,
        // 3815, 3789, 3791, 3777, 3781, 3779, 3793, 3783, 3785, 3787, 3813, 3773,
        // 3769, 3771, 3743, 3737, 3741, 3739, 3837, 3735, 3839, 3835, 3775, 3825,
        // 3823, 3799, 3821, 3819, 3803, 3817, 3807, 3829, 3797, 3827, 3801, 3833,
        // 3831, 3795, 3805, 3615, 3645, 3643, 3647, 3613, 3915, 3845, 3843, 3841,
        // 3913, 3853, 3875, 3857, 3855, 3847, 3919, 3873, 3917, 3849, 3851, 3911,
        // 3889, 3937, 3887, 3909, 3879, 3881, 3885, 3877, 3883, 3943, 3939, 3907,
        // 3941, 3905, 3921, 3967, 3901, 3859, 3945, 3865, 3955, 3861, 3871, 3863,
        // 3867, 3903, 3963, 3899, 3895, 3897, 3869, 3965, 3961, 3959, 3891, 3893,
        // 3957, 3931, 3933, 3929, 3949, 3935, 3953, 3927, 3947, 3951, 3925, 3923,
        // 4013, 4033, 4035, 4015, 3971, 3973, 4043, 4045, 4047, 4041, 4019, 4017,
        // 4001, 4037, 3975, 4039, 4011, 4009, 4065, 3979, 3977, 3981, 3969, 4003,
        // 3983, 4005, 4007, 4053, 4023, 4031, 4027, 4057, 4029, 4055, 4025, 4021,
        // 4051, 4049, 3985, 3987, 3991, 4071, 4059, 3995, 4061, 4069, 4063, 4067,
        // 3999, 3997, 3993, 3989, 4073, 4087, 4077, 4085, 4083, 4075, 4079, 4089,
        // 4093, 4081, 4091, 4095
    };

    auto top_tree = storage.load_top_tree();
    std::cout << "top_tree bounds: " << top_tree.bounds() << "\n\n";

    for(auto i : subtree_ids) {
        auto subtree = storage.load_subtree(i);

        auto inner_bounds = std::array<Point3Dx, 2>{
            Point3Dx{
                std::numeric_limits<CoordType>::max(),
                std::numeric_limits<CoordType>::max(),
                std::numeric_limits<CoordType>::max(),
            },
            Point3Dx{
                std::numeric_limits<CoordType>::lowest(),
                std::numeric_limits<CoordType>::lowest(),
                std::numeric_limits<CoordType>::lowest(),
            }
        };

        for(const auto &value : subtree) {
            inner_bounds[0] = min(inner_bounds[0], get_centroid(value));
            inner_bounds[1] = max(inner_bounds[1], get_centroid(value));
        }

        std::cout << "outer bounds: " << subtree.bounds() << "\n"
                  << "inner bounds: " << Box3D{inner_bounds[0], inner_bounds[1]} << std::endl;
    }
}
}
