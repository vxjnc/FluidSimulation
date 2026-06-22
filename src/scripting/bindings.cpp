#ifdef SCRIPTING_AVAILABLE

#include <nanobind/nanobind.h>

namespace nb = nanobind;

NB_MODULE(fluidsim, m) { m.doc() = "FluidSimulation scripting API"; }

#endif
