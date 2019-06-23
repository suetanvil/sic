// This file is part of Sic; Copyright (C) 2019 The Author(s)
// LGPLv2 w/ exemption; NO WARRANTY! See Copyright.txt for details

# pragma once

#include <sic.hpp>

namespace sic {

void add_test_functions(context *ctx);
int tests_run(context *ctx);
bool success(context *ctx);
long failures(context *ctx);

};
