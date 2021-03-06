// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <sstream>

#include "motive/anim.h"

namespace motive {

void RigAnim::Init(const char* anim_name, BoneIndex num_bones,
                   bool record_names) {
  assert(num_bones <= kMaxNumBones);
  anims_.resize(num_bones);
  bone_parents_.resize(num_bones);
  if (record_names) {
    bone_names_.resize(num_bones);
    if (anim_name != nullptr) {
      anim_name_ = anim_name;
    }
  }
}

MatrixAnim& RigAnim::InitMatrixAnim(BoneIndex idx, BoneIndex parent,
                                    const char* bone_name) {
  assert(idx < static_cast<int>(anims_.size()));
  assert(parent < idx || parent == kInvalidBoneIdx);
  bone_parents_[idx] = static_cast<uint8_t>(parent);
  if (bone_names_.size() > 0) {
    bone_names_[idx] = bone_name;
  }
  return anims_[idx];
}

int RigAnim::NumOps() const {
  size_t num_ops = 0;
  for (BoneIndex i = 0; i < NumBones(); ++i) {
    const MatrixOpArray::OpVector& ops = anims_[i].ops().ops();
    num_ops += ops.size();
  }
  return static_cast<int>(num_ops);
}

std::string RigAnim::CsvHeaderForDebugging(int line) const {
  std::ostringstream oss;

  // Output the bone names, and gaps for where that bone's ops will go.
  for (BoneIndex i = 0; i < NumBones(); ++i) {
    const MatrixOpArray::OpVector& ops = anims_[i].ops().ops();
    if (ops.size() == 0) continue;

    if (line == 0) {
      oss << BoneName(i);
    }

    for (size_t j = 0; j < ops.size(); ++j) {
      if (line == 0) {
        oss << ',';
      } else {
        oss << MatrixOpName(ops[j].type) << ",";
      }
    }
  }
  return oss.str();
}

}  // namespace motive
