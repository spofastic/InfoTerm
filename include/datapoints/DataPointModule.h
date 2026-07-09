#pragma once

/******************************************************************************
 * InfoTerm
 * Module  : DataPoint module scaffold
 * Version : 0.6.3
 *
 * This header introduces the namespace for DataPointManager lifecycle helpers,
 * source grouping and diagnostics. Runtime registration/update helpers are
 * now located in src/datapoints/DataPointRuntime.inc as an intermediate
 * migration step while behavior remains unchanged.
 ******************************************************************************/

namespace InfoTermDataPoints {

void begin();
void registerDefaults();
void updateRuntimeValues();
const char* moduleName();
const char* moduleVersion();

}  // namespace InfoTermDataPoints
