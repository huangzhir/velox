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
#include "velox/exec/Expand.h"

namespace facebook::velox::exec {

Expand::Expand(
    int32_t operatorId,
    DriverCtx* driverCtx,
    const std::shared_ptr<const core::ExpandNode>& expandNode)
    : Operator(
          driverCtx,
          expandNode->outputType(),
          operatorId,
          expandNode->id(),
          "Expand") {
  const auto& inputType = expandNode->sources()[0]->outputType();
  auto numRows = expandNode->projections().size();
  rowProjections_.reserve(numRows);
  constantProjections_.reserve(numRows);
  auto numColumns = expandNode->names().size();
  for (const auto& rowProjections : expandNode->projections()) {
    std::vector<column_index_t> rowProjection;
    rowProjection.reserve(numColumns);
    std::vector<ConstantTypedExprPtr> constantProjection;
    constantProjection.reserve(numColumns);
    for (const auto& columnProjection : rowProjections) {
      if (auto field =
              std::dynamic_pointer_cast<const core::FieldAccessTypedExpr>(
                  columnProjection)) {
        rowProjection.push_back(inputType->getChildIdx(field->name()));
        constantProjection.push_back(nullptr);
      } else if (
          auto constant =
              std::dynamic_pointer_cast<const core::ConstantTypedExpr>(
                  columnProjection)) {
        rowProjection.push_back(kUnMapedProject);
        constantProjection.push_back(constant);
      } else {
        VELOX_USER_FAIL(
            "Expand operator doesn't support this expression. Only column references and constants are supported. {}",
            columnProjection->toString())
      }
    }

    rowProjections_.emplace_back(std::move(rowProjection));
    constantProjections_.emplace_back(std::move(constantProjection));
  }
}

bool Expand::needsInput() const {
  return !noMoreInput_ && input_ == nullptr;
}

void Expand::addInput(RowVectorPtr input) {
  // Load Lazy vectors.
  for (auto& child : input->children()) {
    child->loadedVector();
  }

  input_ = std::move(input);
}

RowVectorPtr Expand::getOutput() {
  if (!input_) {
    return nullptr;
  }

  auto numInput = input_->size();

  std::vector<VectorPtr> outputColumns(outputType_->size());

  const auto& rowProjection = rowProjections_[rowIndex_];
  const auto& constantProjection = constantProjections_[rowIndex_];
  auto numColumns = rowProjection.size();

  for (auto i = 0; i < numColumns; ++i) {
    if (rowProjection[i] == kUnMapedProject) {
      const auto& constantExpr = constantProjection[i];
      if (constantExpr->value().isNull()) {
        // Add null column.
        outputColumns[i] = BaseVector::createNullConstant(
            outputType_->childAt(i), numInput, pool());
      } else {
        // Add constant column.
        outputColumns[i] = BaseVector::createConstant(
            constantExpr->type(), constantExpr->value(), numInput, pool());
      }
    } else {
      outputColumns[i] = input_->childAt(rowProjection[i]);
    }
  }

  ++rowIndex_;
  if (rowIndex_ == rowProjections_.size()) {
    rowIndex_ = 0;
    input_ = nullptr;
  }
  return std::make_shared<RowVector>(
      pool(), outputType_, nullptr, numInput, std::move(outputColumns));
}

} // namespace facebook::velox::exec
