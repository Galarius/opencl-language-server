#pragma once

#define __CL_ENABLE_EXCEPTIONS
#pragma warning(push, 0)
#if defined(__APPLE__) || defined(__MACOSX) || defined(WIN32)
    #include "opencl/cl.hpp"
#else
    #include <CL/cl.hpp>
#endif
#pragma warning(pop)