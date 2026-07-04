"""Fluid source control"""

from collections.abc import Sequence
import enum


class FluidSource:
    @property
    def color(self) -> list[float]: ...

    @color.setter
    def color(self, arg: Sequence[float], /) -> None: ...

    @property
    def x(self) -> float: ...

    @x.setter
    def x(self, arg: float, /) -> None: ...

    @property
    def y(self) -> float: ...

    @y.setter
    def y(self, arg: float, /) -> None: ...

    @property
    def vx(self) -> float: ...

    @vx.setter
    def vx(self, arg: float, /) -> None: ...

    @property
    def vy(self) -> float: ...

    @vy.setter
    def vy(self, arg: float, /) -> None: ...

    @property
    def radius(self) -> float: ...

    @radius.setter
    def radius(self, arg: float, /) -> None: ...

    @property
    def active(self) -> bool: ...

    @active.setter
    def active(self, arg: bool, /) -> None: ...

    @property
    def mask(self) -> Mode: ...

    @mask.setter
    def mask(self, arg: Mode, /) -> None: ...

    @property
    def form(self) -> Form: ...

    @form.setter
    def form(self, arg: Form, /) -> None: ...

class Mode(enum.Flag):
    VELOCITY = 1

    DYE_ADDITIVE = 2

    DYE_REPLACE = 4

class Form(enum.Enum):
    CIRCLE = 0

    LINE = 1

    RADIAL = 2

def add_source(x: float, y: float, vx: float, vy: float, radius: float, color: Sequence[float]) -> FluidSource: ...

def remove_source(idx: int) -> None: ...

def get_source(idx: int) -> FluidSource: ...

def get_sources() -> list: ...
