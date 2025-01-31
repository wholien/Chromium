// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/paint/TransformPaintPropertyNode.h"

#include "platform/graphics/paint/PropertyTreeState.h"

namespace blink {

TransformPaintPropertyNode* TransformPaintPropertyNode::root() {
  DEFINE_STATIC_REF(TransformPaintPropertyNode, root,
                    (TransformPaintPropertyNode::create(
                        nullptr, TransformationMatrix(), FloatPoint3D(),
                        ScrollPaintPropertyNode::root())));
  return root;
}

// TODO(pdr): print out scroll node information.
String TransformPaintPropertyNode::toString() const {
  return String::format(
      "parent=%p transform=%s origin=%s flattensInheritedTransform=%s "
      "renderingContextId=%x directCompositingReasons=%s "
      "compositorElementId=(%d, %d)",
      m_parent.get(), m_matrix.toString().ascii().data(),
      m_origin.toString().ascii().data(),
      m_flattensInheritedTransform ? "yes" : "no", m_renderingContextId,
      compositingReasonsAsString(m_directCompositingReasons).ascii().data(),
      m_compositorElementId.primaryId, m_compositorElementId.secondaryId);
}

#if DCHECK_IS_ON()

String TransformPaintPropertyNode::toTreeString() const {
  return blink::PropertyTreeStatePrinter<blink::TransformPaintPropertyNode>()
      .pathAsString(this);
}

#endif

}  // namespace blink
