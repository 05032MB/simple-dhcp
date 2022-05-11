#pragma once

#include <string.h>
#include <stdexcept>
#include <string>

#define THROW_RUNTIME_GET_ERRNO(what) \
    throw std::runtime_error( what ": " + std::string(strerror(errno)) );

