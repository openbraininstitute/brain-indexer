import logging
from abc import abstractmethod


class ChunckedProcessingMixin:

    N_ELEMENTS_CHUNK = 100

    @abstractmethod
    def n_elements_to_import(self):
        return NotImplemented

    @abstractmethod
    def process_range(self, range_):
        return NotImplemented

    def process_all(self):
        n_elements = self.n_elements_to_import()
        nchunks = (n_elements - 1) // self.N_ELEMENTS_CHUNK + 1

        for i, range_ in enumerate(gen_ranges(n_elements, self.N_ELEMENTS_CHUNK)):
            self.process_range(range_)
            show_progress(i + 1, nchunks)

    @classmethod
    def create(cls, *args):
        """Interactively create, with some progress"""
        indexer = cls(*args)
        indexer.process_all()
        return indexer

    @classmethod
    def create_parallel(cls, *ctor_args, num_cpus=None):
        import functools
        import os
        import multiprocessing
        if num_cpus is None:
            num_cpus_env = os.environ.get("SLURM_TASKS_PER_NODE")
            num_cpus = int(num_cpus_env) if num_cpus_env else multiprocessing.cpu_count()

        # make indexer global, so that in each runner process, among processing chunks,
        # morphologies dont get deleted
        indexer = globals()["indexer"] = cls(*ctor_args)
        n_elements = indexer.n_elements_to_import()
        nchunks = int(n_elements) / cls.N_ELEMENTS_CHUNK
        ranges = gen_ranges(n_elements, cls.N_ELEMENTS_CHUNK)
        # use functools as lambdas are not serializable
        build_index = functools.partial(_process_range_increment, cls, ctor_args)

        logging.info("Running in parallel. CPUs=" + str(num_cpus))
        with multiprocessing.Pool(num_cpus) as pool:
            for i, _ in enumerate(pool.imap_unordered(build_index, ranges)):
                show_progress(i + 1, nchunks)
        return indexer  # the indexer on rank0


def _process_range_increment(cls, ctor_args, part):
    # Instantiate indexer just once per process
    indexer = globals().get("indexer")
    if not indexer:
        logging.debug("No cached indexer. Building new... %s", ctor_args)
        indexer = globals()["indexer"] = cls(*ctor_args)
    indexer.process_range(part)


def gen_ranges(limit, blocklen, low=0):
    for high in range(low + blocklen, limit, blocklen):
        yield low, high
        low = high
    if low < limit:
        yield low, limit


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


def show_progress(iteration, total, prefix='Progress:', decimals=1, length=80, fill='█'):
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    print(f'\r{prefix} |{bar}| {percent}% ', end="   ")
    # Print New Line on Complete
    if iteration == total:
        print()
