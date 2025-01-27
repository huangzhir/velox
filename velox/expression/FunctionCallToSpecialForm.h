/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "velox/expression/Expr.h"
#include "velox/type/Type.h"

namespace facebook::velox::exec {
class FunctionCallToSpecialForm {
 public:
  virtual ~FunctionCallToSpecialForm() {}

  /// Returns the output Type of the SpecialForm given the input argument Types.
  /// Throws if the input Types do not match what's expected for the SpecialForm
  /// or if the SpecialForm cannot infer the return Type based on the input
  /// arguments, e.g. Try.
  virtual TypePtr resolveType(const std::vector<TypePtr>& argTypes) = 0;

  /// Returns the output Type of the SpecialForm given the input argument Types.
  /// Return nullptr if the input Types do not match what's expected for the
  /// SpecialForm or if the SpecialForm can infer the return Type based on the
  /// input arguments `std::vector<TypePtr>& argTypes`, e.g. coalesce. Custom
  /// registered SpecialForm may need to evaluate the input argument to get the
  /// output type, e.g. decimal_round.
  virtual TypePtr resolveType(
      const std::vector<std::shared_ptr<const core::ITypedExpr>>& inputs) {
    return nullptr;
  }

  /// Given the output Type, the child expresssions, and whether or not to track
  /// CPU usage, returns the SpecialForm.
  virtual ExprPtr constructSpecialForm(
      const TypePtr& type,
      std::vector<ExprPtr>&& compiledChildren,
      bool trackCpuUsage,
      const core::QueryConfig& config) = 0;
};

/// Returns the output Type of the SpecialForm associated with the functionName
/// given the input argument Types. If functionName is not the name of a known
/// SpecialForm, returns nullptr. Note that some SpecialForms may throw on
/// invalid arguments or if they don't support type resolution, e.g. Try. Some
/// SpecialForms may return null if they should resolve type by `ITypedExpr
/// inputs`
TypePtr resolveTypeForSpecialForm(
    const std::string& functionName,
    const std::vector<TypePtr>& argTypes);

/// Returns the output Type of the SpecialForm associated with the functionName
/// given the input argument Types. If functionName is not the name of a known
/// SpecialForm, returns nullptr. Note that most of SpecialForms may return
/// nullptr if they don't support this type resolution, e.g. coalesce. Custom
/// registered SpecialForm may need to evaluate the input argument to get the
/// output type, e.g. decimal_round.
TypePtr resolveTypeForSpecialForm(
    const std::string& functionName,
    const std::vector<std::shared_ptr<const core::ITypedExpr>>& inputs);

/// Returns the SpeicalForm associated with the functionName.  If functionName
/// is not the name of a known SpecialForm, returns nulltpr.
ExprPtr constructSpecialForm(
    const std::string& functionName,
    const TypePtr& type,
    std::vector<ExprPtr>&& compiledChildren,
    bool trackCpuUsage,
    const core::QueryConfig& config);

/// Returns true iff a FunctionCallToSpeicalForm object has been registered for
/// the given functionName.
bool isFunctionCallToSpecialFormRegistered(const std::string& functionName);
} // namespace facebook::velox::exec
