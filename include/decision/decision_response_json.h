#pragma once

#include "decision/decision_contract.h"

#include <string>

namespace economy {

// Parse one complete decision.v1 response. Every value is a JSON string and
// the five specified fields are the entire permitted object shape.
DecisionResponse parseDecisionResponseJson(const std::string& json);

} // namespace economy
