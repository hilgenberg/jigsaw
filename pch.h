#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#define _USE_MATH_DEFINES
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <time.h>

#ifndef _WINDOWS
#include <unistd.h>
#include <sys/time.h>
#include <sys/param.h>
//#include <pthread.h>
#endif

#ifdef LINUX
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#ifdef ANDROID
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <GLES3/gl32.h>
#endif

#include "Utility/StringFormatting.h"

using std::isnan;
using std::isinf;
using std::isfinite;

