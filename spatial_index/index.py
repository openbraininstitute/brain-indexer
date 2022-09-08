import abc
import functools

import libsonata

import spatial_index
from .io import write_sonata_meta_data_section
from .util import is_non_string_iterable, strip_singleton_non_string_iterable


class IndexInterface(abc.ABC):
    @abc.abstractmethod
    def window_query(self, corner, opposite_corner, *,
                     fields=None, accuracy=None,
                     populations=None, population_mode=None):
        """Find all elements intersecting with the query box.

        A detailed explanation is available in the User Guide.

        Arguments:
            fields(str,list):  A string or iterable of strings specifying which
                attributes of the index are to be returned.

            accuracy(str):     Specifies the accuracy with which indexed
                elements are treated. Allowed are either ``"bounding_box"`` or
                ``"best_effort"``. Default: ``"bounding_box"``

            populations(str,list):  A string or list of strings specifying which
                populations to query. Ignored by single-population indexes.

            population_mode(str):  (advanced) Defines if the query uses the
                single- or multi-population return type. Available: ``None``
                (native), ``"single"`` (single-population), ``"multi"``
                (multi-population). Please consult the User Guide for a detailed
                explanation.
        """
        pass

    @abc.abstractmethod
    def vicinity_query(self, center, radius, *,
                       fields=None, accuracy=None,
                       populations=None, population_mode=None):
        """Find all elements intersecting with the query sphere.

        A detailed explanation is available in the User Guide.

        Arguments:
            fields(str,list):  A string or iterable of strings specifying which
                attributes of the index are to be returned.

            accuracy(str):     Specifies the accuracy with which indexed
                elements are treated. Allowed are either ``"bounding_box"`` or
                ``"best_effort"``. Default: ``"bounding_box"``

            populations(str,list):  A string or list of strings specifying which
                populations to query. Ignored by single-population indexes.

            population_mode(str):  (advanced) Defines if the query uses the
                single- or multi-population return type. Available: ``None``
                (native), ``"single"`` (single-population), ``"multi"``
                (multi-population). Please consult the User Guide for a detailed
                explanation.
        """
        pass

    @abc.abstractmethod
    def window_counts(self, corner, opposite_corner, *,
                      accuracy=None, group_by=None,
                      populations=None, population_mode=None):
        """Counts all elements intersecting with the query box.

        A detailed explanation is available in the User Guide.

        Arguments:
            accuracy(str):  Specifies the accuracy with which indexed
                elements are treated. Allowed are either ``"bounding_box"`` or
                ``"best_effort"``. Default: ``"bounding_box"``

            group_by(str):  Enables first grouping the index elements and then
                counting the number of elements in each group.

            populations(str,list):  A string or list of strings specifying which
                populations to query. Ignored by single-population indexes.

            population_mode(str):  (advanced) Defines if the query uses the
                single- or multi-population return type. Available: ``None``
                (native), ``"single"`` (single-population), ``"multi"``
                (multi-population). Please consult the User Guide for a detailed
                explanation.
        """
        pass

    @abc.abstractmethod
    def vicinity_counts(self, center, radius, *,
                        accuracy=None, group_by=None,
                        populations=None, population_mode=None):
        """Counts all elements intersecting with the query sphere.

        A detailed explanation is available in the User Guide.

        Arguments:
            accuracy(str):  Specifies the accuracy with which indexed
                elements are treated. Allowed are either ``"bounding_box"`` or
                ``"best_effort"``. Default: ``"bounding_box"``

            group_by(str):  Enables first grouping the index elements and then
                counting the number of elements in each group.

            populations(str,list):  A string or list of strings specifying which
                populations to query. Ignored by single-population indexes.

            population_mode(str):  (advanced) Defines if the query uses the
                single- or multi-population return type. Available: ``None``
                (native), ``"single"`` (single-population), ``"multi"``
                (multi-population). Please consult the User Guide for a detailed
                explanation.
        """
        pass

    @abc.abstractmethod
    def bounds(self, populations=None, population_mode=None):
        """The joint minimal bounding box of all elements in the index.

        Arguments:
            populations(str,list):  A string or list of strings specifying which
                populations to query. Ignored by single-population indexes.

            population_mode(str):  (advanced) Defines if the query uses the
                single- or multi-population return type. Available: ``None``
                (native), ``"single"`` (single-population), ``"multi"``
                (multi-population). Please consult the User Guide for a detailed
                explanation.
        """
        pass

    @property
    @abc.abstractmethod
    def available_fields(self):
        """All attributes that can be passed to `fields=`.

        In addition to the builtin fields these may contain the names
        the SONATA attributes that can be retrieved.
        """
        pass

    @property
    @abc.abstractmethod
    def builtin_fields(self):
        """The attributes built into the index.

        These attributes are stored directly inside the spatial index.
        Therefore, you can expect good performance for these fields.
        Furthermore, the builtin fields only depend on the type of index, e.g.
        "morphology", "synapse", etc.
        """
        pass

    @property
    @abc.abstractmethod
    def populations(self):
        """The names of the populations covered by this index.

        Single population indexes may return `[None]` instead of the
        actual population name.
        """
        pass


def _wrap_single_as_multi_population(func):
    def wrapped_func(self, *query_shape, populations=None, population_mode=None,
                     **kwargs):

        if population_mode is None or population_mode == "single":
            return func(self, *query_shape, **kwargs)

        else:
            pop = strip_singleton_non_string_iterable(populations)
            return {pop: func(self, *query_shape, **kwargs)}

    return wrapped_func


class Index(IndexInterface):
    def __init__(self, core_index):
        self._core_index = core_index

        self._window_queries = {
            "_np": self._core_index.find_intersecting_window_np,
            "raw_elements": self._core_index.find_intersecting_window_objs,
        }

        self._vicinity_queries = {
            "_np": self._core_index.find_intersecting_np,
            "raw_elements": self._core_index.find_intersecting_objs,
        }

        self._window_counts = {
            None: self._core_index.count_intersecting,
        }

        if hasattr(self._core_index, "count_intersecting_agg_gid"):
            self._window_counts["gid"] = getattr(
                self._core_index,
                "count_intersecting_agg_gid"
            )

        self._vicinity_counts = {
            None: self._core_index.count_intersecting_vicinity,
        }

        if hasattr(self._core_index, "count_intersecting_vicinity_agg_gid"):
            self._vicinity_counts["gid"] = getattr(
                self._core_index,
                "count_intersecting_vicinity_agg_gid"
            )

    @_wrap_single_as_multi_population
    def window_query(self, corner, opposite_corner, *,
                     fields=None, accuracy=None):
        return self._query(
            (corner, opposite_corner),
            fields=fields,
            accuracy=accuracy,
            methods=self._window_queries,
        )

    @_wrap_single_as_multi_population
    def vicinity_query(self, center, radius, *,
                       fields=None, accuracy=None):
        return self._query(
            (center, radius),
            fields=fields,
            accuracy=accuracy,
            methods=self._vicinity_queries
        )

    @_wrap_single_as_multi_population
    def window_counts(self, corner, opposite_corner, *,
                      group_by=None, accuracy=None):
        return self._counts(
            (corner, opposite_corner),
            group_by=group_by,
            accuracy=accuracy,
            methods=self._window_counts,
        )

    @_wrap_single_as_multi_population
    def vicinity_counts(self, center, radius, *,
                        group_by=None, accuracy=None):
        return self._counts(
            (center, radius),
            group_by=group_by,
            accuracy=accuracy,
            methods=self._vicinity_counts
        )

    def __len__(self):
        return len(self._core_index)

    @_wrap_single_as_multi_population
    def bounds(self):
        return self._core_index.bounds()

    @property
    def builtin_fields(self):
        return list(self._core_index.builtin_fields)

    @property
    def available_fields(self):
        return self.builtin_fields

    @property
    def populations(self):
        return [None]

    def _query(self, query_shape, *, fields=None, accuracy=None, methods=None):
        fields = self._enforce_fields_default(fields)
        accuracy = self._enforce_accuracy_default(accuracy)

        if is_non_string_iterable(fields):
            return self._multi_field_window_query(
                query_shape,
                fields=fields,
                accuracy=accuracy,
                methods=methods
            )

        else:
            return self._single_field_window_query(
                query_shape,
                field=fields,
                accuracy=accuracy,
                methods=methods
            )

    def _multi_field_window_query(self, query_shape, *,
                                  fields=None, accuracy=None, methods=None):

        result = methods["_np"](*query_shape, geometry=accuracy)
        return {k: result[k] for k in fields}

    def _single_field_window_query(self, query_shape, *,
                                   field=None, accuracy=None, methods=None):

        if field in methods:
            return methods[field](*query_shape, geometry=accuracy)

        else:
            result = methods["_np"](*query_shape, geometry=accuracy)
            return result[field]

    def _enforce_accuracy_default(self, accuracy):
        if accuracy is None:
            return "bounding_box"

        return accuracy

    def _enforce_fields_default(self, fields):
        if fields is None:
            fields = self.builtin_fields

        # must catch: "" and any empty iterator.
        if len(fields) == 0:
            raise ValueError(f"Invalid fields: {fields}")

        return fields

    def _counts(self, query_shape, *,
                group_by=None, accuracy=None, methods=None):

        method = methods.get(group_by, None)
        if method is None:
            raise ValueError(f"Unsupported argument: group_by={group_by}")

        accuracy = self._enforce_accuracy_default(accuracy)
        return method(*query_shape, geometry=accuracy)


class SynapseIndexBase(Index):
    def __init__(self, core_index, sonata_edges=None):
        super().__init__(core_index)

        self._available_fields = self.builtin_fields

        if sonata_edges is not None:
            self._sonata_edges = sonata_edges
            self._multi_field_window_query = self._sonata_multi_field_window_query
            self._single_field_window_query = self._sonata_single_field_window_query

            self._available_fields += self._sonata_edges.attribute_names

    @property
    def available_fields(self):
        return list(self._available_fields)

    def _sonata_multi_field_window_query(self, query_shape, *,
                                         fields=None, accuracy=None, methods=None):

        available_builtin_fields = self.builtin_fields
        special_fields = self._deduce_special_fields(methods)

        builtin_fields = filter(
            lambda f: f in available_builtin_fields,
            set(fields).union(["id"])
        )
        sonata_fields = set(fields).difference(available_builtin_fields + special_fields)

        if any(f in special_fields for f in fields):
            spatial_index.logger.error(
                "The special fields: \n"
                + f"  {special_fields}"
                + "can't be used in multi-field queries. Please query them\n"
                + "one by one for now."
            )

            raise ValueError(f"Invalid fields: {fields}")

        result = super()._multi_field_window_query(
            query_shape, fields=builtin_fields, accuracy=accuracy, methods=methods
        )

        if sonata_fields:
            selection = libsonata.Selection(result['id'])

            for field in sonata_fields:
                result[field] = self._sonata_query(selection=selection, field=field)

        return {k: result[k] for k in fields}

    def _sonata_single_field_window_query(self, query_shape, *,
                                          field=None, accuracy=None, methods=None):

        regular_fields = self.builtin_fields + self._deduce_special_fields(methods)

        if field in regular_fields:
            return super()._single_field_window_query(
                query_shape, field=field, accuracy=accuracy, methods=methods
            )

        else:
            ids = super()._single_field_window_query(
                query_shape, field="id", accuracy=accuracy, methods=methods,
            )

            selection = libsonata.Selection(ids)
            return self._sonata_query(selection=selection, field=field)

    def _deduce_special_fields(self, methods):
        return [key for key in methods.keys() if key != "_np"]

    def _sonata_query(self, *, selection, field):
        return self._sonata_edges.get_attribute(field, selection)

    @classmethod
    def from_meta_data(cls, meta_data, **kwargs):
        core_index = cls._open_core_from_meta_data(meta_data, **kwargs)

        if extended_conf := meta_data.extended:
            sonata_edges = spatial_index.io.open_sonata_edges(
                extended_conf.path("dataset_path"),
                extended_conf.value("population")
            )
            return cls(core_index, sonata_edges)

        else:
            return cls(core_index)

    @classmethod
    def _open_core_from_meta_data(cls, meta_data, **kwargs):
        return spatial_index.io.open_core_from_meta_data(
            meta_data, resolver=spatial_index.SynapseIndexResolver, **kwargs
        )

    @property
    def _element_type(self):
        return "synapse"


class SynapseIndex(SynapseIndexBase):
    def write(self, index_path, *, sonata_filename=None, population=None):
        """Saves the index to disk.

        If both `sonata_filename` and `population` are passed, then the
        additional metadata needed to load an index supporting fetching
        attributes from SONATA is also saved.

        No action is performed if `index_path` is `None`.
        """
        if index_path is not None:
            self._core_index.dump(index_path)

            if sonata_filename is not None and population is not None:
                write_sonata_meta_data_section(
                    index_path, sonata_filename, population
                )


class SynapseMultiIndex(SynapseIndexBase):
    pass


class _FromMetaDataWithOutSonata:
    @classmethod
    def from_meta_data(cls, meta_data, **kwargs):
        return cls(
            spatial_index.io.open_core_from_meta_data(
                meta_data, resolver=cls._resolver(), **kwargs
            )
        )


class MorphIndexBase(Index, _FromMetaDataWithOutSonata):
    @classmethod
    def _resolver(cls):
        return spatial_index.MorphIndexResolver

    @property
    def _element_type(self):
        return "morphology"


class _WriteInMemoryIndex:
    def write(self, index_path):
        """Saves the index to disk.

        No action is performed if `index_path` is `None`.
        """
        if index_path is not None:
            self._core_index.dump(index_path)


class MorphIndex(MorphIndexBase, _WriteInMemoryIndex):
    pass


class MorphMultiIndex(MorphIndexBase):
    pass


class SphereIndexBase(Index, _FromMetaDataWithOutSonata):
    @property
    def _element_type(self):
        return "sphere"


class SphereIndex(SphereIndexBase, _WriteInMemoryIndex):
    pass


# TODO integrage other types of indexes like `PointIndex` and `SphereIndex`


def _wrap_as_multi_population(func):
    @functools.wraps(func)
    def _multi_pop_func(self, *args, population_mode=None, populations=None, **kwargs):
        populations = self._deduce_populations(populations)

        if population_mode == "single":
            if len(populations) != 1:
                raise ValueError(
                    f"Invalid argument {populations=} for single population mode."
                )
            pop = next(iter(populations))
            return func(self, self._indexes[pop], *args, **kwargs)

        return {
            pop: func(self, self._indexes[pop], *args, **kwargs) for pop in populations
        }

    return _multi_pop_func


class MultiPopulationIndex(IndexInterface):
    def __init__(self, indexes):
        self._indexes = indexes

    @_wrap_as_multi_population
    def vicinity_query(self, index, *args, **kwargs):
        return index.vicinity_query(*args, **kwargs)

    @_wrap_as_multi_population
    def window_query(self, index, *args, **kwargs):
        return index.window_query(*args, **kwargs)

    @_wrap_as_multi_population
    def window_counts(self, index, *args, **kwargs):
        return index.window_counts(*args, **kwargs)

    @_wrap_as_multi_population
    def vicinity_counts(self, index, *args, **kwargs):
        return index.vicinity_counts(*args, **kwargs)

    @_wrap_as_multi_population
    def bounds(self, index, *args, **kwargs):
        return index.bounds(*args, **kwargs)

    def _deduce_populations(self, populations):
        if populations is None:
            return list(self._indexes.keys())

        if not is_non_string_iterable(populations):
            populations = [populations]

        return populations

    @property
    def available_fields(self):
        return next(iter(self._indexes.values())).available_fields

    @property
    def builtin_fields(self):
        return next(iter(self._indexes.values())).builtin_fields

    @property
    def populations(self):
        return list(self._indexes.keys())

    @property
    def _element_type(self):
        return next(iter(self._indexes.values()))._element_type
