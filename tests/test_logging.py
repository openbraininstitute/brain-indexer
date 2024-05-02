import logging
import tempfile
import os

import brain_indexer


def test_register_logger():
    info_message_py = "eiwooi"
    warn_message_py = "nvueir"
    error_message_py = "xieowu"

    debug_message_cpp = "bhuiew"
    cond_debug_message_cpp = "uerou"
    info_message_cpp = "woqoie"
    warn_message_cpp = "hguwps"
    error_message_cpp = "qasdjw"

    with tempfile.TemporaryDirectory(prefix="test_si_logging") as d:
        filename = os.path.join(d, "test_si_logging.log")

        file_logger = logging.getLogger("test_register_logger")
        file_logger.setLevel(logging.DEBUG)
        file_logger.addHandler(logging.FileHandler(filename))

        brain_indexer.register_logger(file_logger)

        brain_indexer.logger.info(info_message_py)
        brain_indexer.logger.warning(warn_message_py)
        brain_indexer.logger.error(error_message_py)

        brain_indexer.core.tests.write_logs(
            debug_message_cpp,
            cond_debug_message_cpp,
            info_message_cpp,
            warn_message_cpp,
            error_message_cpp
        )

        with open(filename, "r") as f:
            log = f.read()

        assert info_message_py in log
        assert warn_message_py in log
        assert error_message_py in log

        assert debug_message_cpp in log
        assert cond_debug_message_cpp in log
        assert info_message_cpp in log
        assert warn_message_cpp in log
        assert error_message_cpp in log
