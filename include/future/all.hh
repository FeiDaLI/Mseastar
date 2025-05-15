#pragma once

// Include in dependency order to avoid problems
#include "future_state.hh"  // Most basic component with no dependencies
#include "future.hh"        // Depends on future_state
#include "promise.hh"       // Depends on future and future_state
#include "futurize.hh"      // Depends on all of the above

// This header can be used when you need the complete future system 