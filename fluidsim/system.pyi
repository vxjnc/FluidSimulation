"""Notifications, dialogs, tick scheduling"""

from collections.abc import Callable, Sequence
import enum


def on_tick(callback: Callable[[], None] | None) -> None: ...

def open_file_dialog(filters: Sequence[tuple[str, str]] | None = None) -> str | None: ...

def save_file_dialog(filters: Sequence[tuple[str, str]] | None = None, default_name: str) -> str | None: ...

class NotifyLevel(enum.Enum):
    INFO = 0

    WARNING = 1

    ERROR = 2

def notify(level: NotifyLevel, message: str) -> None: ...

def notify_error(message: str) -> None: ...

def notify_warning(message: str) -> None: ...

def notify_info(message: str) -> None: ...
