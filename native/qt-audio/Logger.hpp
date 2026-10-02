// Route upstream audio logs to stderr without its Boost logging subsystem.
#pragma once
#include <cstdio>
#include <sstream>
#define LOG_WARN(ARG) do { std::ostringstream message; message << ARG; fprintf(stderr,"%s\n",message.str().c_str()); } while(false)
#define LOG_ERROR(ARG) LOG_WARN(ARG)
