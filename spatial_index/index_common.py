# This file is part of SpatialIndex, the new-gen spatial indexer for BBP
# Copyright Blue Brain Project 2020-2021. All rights reserved


class DiskMemMapProps:
    """A class to configure memory-mapped files as the backend for spatial indices."""

    def __init__(self, map_file, file_size=1024, close_shrink=False):
        self.memdisk_file = map_file
        self.file_size = file_size
        self.shrink = close_shrink

    @property
    def args(self):
        return self.memdisk_file, self.file_size, self.shrink
