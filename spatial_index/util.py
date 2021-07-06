from abc import abstractmethod


class ChunckedProcessingMixin:

    N_CELLS_RANGE = 100

    @abstractmethod
    def cell_count(self):
        return NotImplemented

    @abstractmethod
    def process_range(self, range_):
        return NotImplemented

    @classmethod
    def icreate(cls, *args):
        """Interactively create, with some progress"""
        indexer = cls(*args)
        n_cells = indexer.cell_count()
        nranges = int(n_cells / cls.N_CELLS_RANGE)

        for i, range_ in enumerate(gen_ranges(n_cells, cls.N_CELLS_RANGE)):
            indexer.process_range(range_)
            print(" - Processed {:.0f}%".format(float(i+1) * 100 / nranges))

    @classmethod
    def create_parallel(cls, *ctor_args, num_cpus=None):
        import functools
        import os
        import multiprocessing
        if num_cpus is None:
            num_cpus_env = os.environ.get("SLURM_TASKS_PER_NODE")
            num_cpus = int(num_cpus_env) if num_cpus_env else multiprocessing.cpu_count()

        indexer = globals()["indexer"] = cls(*ctor_args)
        n_cells = len(indexer.mvd)
        nranges = int(n_cells) / cls.N_CELLS_RANGE
        ranges = gen_ranges(n_cells, cls.N_CELLS_RANGE)
        # use functools as lambdas are not serializable
        build_index = functools.partial(_process_range_increment, cls, ctor_args)

        print("Running in parallel. CPUs=" + str(num_cpus))
        with multiprocessing.Pool(num_cpus) as pool:
            for i, _ in enumerate(pool.imap_unordered(build_index, ranges)):
                print(" - Processed {:.0f}%".format(float(i+1) * 100 / nranges))


def _process_range_increment(cls, ctor_args, part):
    # Instantiate indexer just once per process
    indexer = globals().get("indexer")
    if not indexer:
        indexer = globals()["indexer"] = cls(*ctor_args)
    indexer.process_range(part)


def gen_ranges(limit, blocklen):
    low_i = 0
    for high_i in range(blocklen, limit, blocklen):
        yield low_i, high_i - low_i
        low_i = high_i
    if low_i < limit:
        yield low_i, limit - low_i


def docopt_get_args(func, extra_args=None):
    """Get all CLI  args via docopt
    """
    from docopt import docopt
    docopt_opts = docopt(func.__doc__, extra_args)
    opts = {}
    for key, val in docopt_opts.items():
        key = key.strip("<>-").replace("-", "_")
        if isinstance(val, str):
            if val.lower() in ("off", "false"):
                val = False
            elif val.lower() in ("on", "true"):
                val = True
        opts[key] = val
    return opts
