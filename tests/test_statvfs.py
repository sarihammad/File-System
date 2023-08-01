import os
import pytest

import expected_values

BLOCK_SIZE = 4096
MAX_FILE_LENGTH = 252


@pytest.fixture()
def stats(mount_point: str) -> os.statvfs_result:
    """Return the filesystem statistics obtained by a statvfs call on mount_point."""
    return os.statvfs(mount_point)


def test_f_bsize(stats: os.statvfs_result) -> None:
    """Test that filesystem has the expected block size."""
    assert stats.f_bsize == BLOCK_SIZE


def test_f_frsize(stats: os.statvfs_result) -> None:
    """Test that filesystem has the expected fragment size."""
    assert stats.f_frsize == BLOCK_SIZE


def test_f_files(stats: os.statvfs_result, inode_count: int) -> None:
    """Test that filesystem has the expected number of inodes."""
    assert stats.f_files == inode_count


def test_f_namemax(stats: os.statvfs_result) -> None:
    """Test that filesystem has the expected maximum filename length."""
    assert stats.f_namemax == MAX_FILE_LENGTH


def test_every_statvfs_attribute(stats: os.statvfs_result, disk: str) -> None:
    """Test that filesystem has the expected value for every attribute in a statvfs result.
    """
    assert disk in expected_values.STATVFS, 'The disk argument does not have any expected values.'

    expected = expected_values.STATVFS[disk]
    assert stats == expected
