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


def setup_logging_for_cli(verbose):
    """Configure logging when SI is used as an application.

    When running SI as a standalone application, we control the
    format of the log messages. This function sets up the `logging`
    library, including the root logger.

    This is needed to be able to set a log level other than `WARN`.
    Failing to configure the root logger, implies a fallback logger,
    which effectively simply ignores our `logger.setLevel`.

    When setting `verbose` to `True`, the log level is set to DEBUG.
    """
    from spatial_index import logger

    logging.basicConfig()

    if verbose:
        logger.setLevel(logging.DEBUG)
