/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef INC_COMMON_POST_UTIL_H_
#define INC_COMMON_POST_UTIL_H_

#include "common/inc/global/Framework.h"
#include "inc/common/post.h"

ReturnStatus _execPost(OCMPMessageFrame *pMsg, unsigned int subsystem_id);

#endif /* INC_COMMON_POST_UTIL_H_ */
