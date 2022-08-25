import logging

from . import _spatial_index as core


def minimum_log_severity():
    """The minimum severity of messages that should be logged."""

    severity = core._minimum_log_severity()
    if severity == core._LogSeverity.DEBUG:
        return logging.DEBUG
    elif severity == core._LogSeverity.INFO:
        return logging.INFO
    elif severity == core._LogSeverity.WARN:
        return logging.WARN
    elif severity == core._LogSeverity.ERROR:
        return logging.ERROR
    else:
        raise NotImplementedError("Unknown log severity.")
