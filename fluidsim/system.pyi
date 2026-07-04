"""Notifications, dialogs, tick scheduling"""

from collections.abc import Callable, Sequence
import enum


def on_tick(callback: Callable[[], None] | None) -> None: ...

def open_file_dialog(filters: Sequence[tuple[str, str]] | None = None) -> str | None: ...

def save_file_dialog(filters: Sequence[tuple[str, str]] | None = None, default_name: str) -> str | None: ...

class NotifyLevel(enum.Enum):
    Info = 0

    Warning = 1

    Error = 2

def notify(level: NotifyLevel, message: str) -> None: ...
