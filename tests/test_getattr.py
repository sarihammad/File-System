import os

import expected_values


def test_st_mode(mount_point: str, disk: str) -> None:
    """Test that every file in the filesystem has the expected file type and mode (i.e., st_mode).

    This test is only useful if disk is in expected_values.STATVFS.
    """
    assert disk in expected_values.STATVFS, f'The disk argument {disk} does not have any expected values.'

    for file in expected_values.FILES[disk]:
        assert file in expected_values.FILE_STATS, f'Could not find stats for {file}'

        path_to_file = os.path.join(mount_point, file)
        actual = os.stat(path_to_file)

        expected = expected_values.FILE_STATS[file]
        assert actual.st_mode == expected.st_mode