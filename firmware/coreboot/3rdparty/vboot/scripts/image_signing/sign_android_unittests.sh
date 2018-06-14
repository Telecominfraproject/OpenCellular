#!/bin/bash

# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

. "$(dirname "$0")/lib/sign_android_lib.sh"

# Expected APK signatures depending on the type of APK and the type of build.
declare -A platform_sha=(
  ['cheets']='AA:04:E0:5F:82:9C:7E:D1:B9:F8:FC:99:6C:5A:54:43:83:D9:F5:BC'
  ['aosp']='27:19:6E:38:6B:87:5E:76:AD:F7:00:E7:EA:84:E4:C6:EE:E3:3D:FA'
)
declare -A media_sha=(
  ['cheets']='D4:C4:2D:E0:B9:1B:15:72:FA:7D:A7:21:E0:A6:09:94:B4:4C:B5:AE'
  ['aosp']='B7:9D:F4:A8:2E:90:B5:7E:A7:65:25:AB:70:37:AB:23:8A:42:F5:D3'
)
declare -A shared_sha=(
  ['cheets']='38:B6:2C:E1:75:98:E3:E1:1C:CC:F6:6B:83:BB:97:0E:2D:40:6C:AE'
  ['aosp']='5B:36:8C:FF:2D:A2:68:69:96:BC:95:EA:C1:90:EA:A4:F5:63:0F:E5'
)
declare -A release_sha=(
  ['cheets']='EC:63:36:20:23:B7:CB:66:18:70:D3:39:3C:A9:AE:7E:EF:A9:32:42'
  ['aosp']='61:ED:37:7E:85:D3:86:A8:DF:EE:6B:86:4B:D8:5B:0B:FA:A5:AF:81'
)


test_android_choose_key_invalid_keyset() {
  local keyname
  local keyset
  local keysets=("invalid_keyset" " " "")

  for keyset in "${keysets[@]}"; do
    echo "TEST: Detection of invalid keyset '${keyset}'."
    if keyname=$(android_choose_key "ignored_sha1" "${keyset}"); then
      : $(( NUM_TEST_FAILURES += 1 ))
      echo "ERROR: Failed to detect invalid keyset '${keyset}'."
    else
      echo "PASS: Detected invalid keyset '${keyset}'."
    fi
  done
}

android_choose_key_test_helper() {
  local sha1="$1"
  local keyset="$2"
  local expected_keyname="$3"
  local keyname="invalid_key"

  echo "TEST: Detect '${expected_keyname}' key name for '${keyset}' keyset."
  keyname=$(android_choose_key "${sha1}" "${keyset}")
  if [[ "${keyname}" != "${expected_keyname}" ]]; then
    : $(( NUM_TEST_FAILURES += 1 ))
    echo "ERROR: Incorrect key name '${keyname}' returned."
  else
    echo "PASS: Correct key name '${keyname}' returned."
  fi
}

test_android_choose_key() {
  local keyset
  local expected_keyname

  local keysets=("cheets" "aosp")
  for keyset in "${keysets[@]}"; do
    expected_keyname="platform"
    android_choose_key_test_helper "${platform_sha[${keyset}]}" "${keyset}" \
      "${expected_keyname}"
    expected_keyname="media"
    android_choose_key_test_helper "${media_sha[${keyset}]}" "${keyset}" \
      "${expected_keyname}"
    expected_keyname="shared"
    android_choose_key_test_helper "${shared_sha[${keyset}]}" "${keyset}" \
      "${expected_keyname}"
    expected_keyname="releasekey"
    android_choose_key_test_helper "${release_sha[${keyset}]}" "${keyset}" \
      "${expected_keyname}"
  done
}

build_flavor_test_helper() {
  local prop_file="${BUILD}/build.prop"
  local prop_content="$1"
  local expected_flavor_prop="$2"
  local flavor_prop=""

  echo "${prop_content}" > "${prop_file}"
  flavor_prop=$(android_get_build_flavor_prop "${prop_file}")
  if [[ "${flavor_prop}" != "${expected_flavor_prop}" ]]; then
    : $(( NUM_TEST_FAILURES += 1 ))
    echo "ERROR: Incorrect build flavor '${flavor_prop}' returned."
  else
    echo "PASS: Correct key name '${flavor_prop}' returned."
  fi
  rm "${prop_file}"
}

test_android_get_build_flavor_prop() {
  local prop_file="${BUILD}/build.prop"
  local prop_content=""
  local flavor_prop=""

  echo "TEST: Extract ro.build.flavor property."
  prop_content="ro.random.prop=foo
other.prop=bar
x=foobar
ro.build.flavor=cheets_x86-user
another.prop=barfoo"
  build_flavor_test_helper "${prop_content}" "cheets_x86-user"

  echo "TEST: Extract single ro.build.flavor property."
  prop_content="ro.build.flavor=cheets_x86-user"
  build_flavor_test_helper "${prop_content}" "cheets_x86-user"

  echo "TEST: Avoid commented out ro.build.flavor property."
  prop_content="ro.random.prop=foo
other.prop=bar
x=foobar
#ro.build.flavor=commented_out
ro.build.flavor=cheets_x86-user
another.prop=barfoo"
  build_flavor_test_helper "${prop_content}" "cheets_x86-user"

  # Missing ro.build.flavor property.
  echo "TEST: Detect missing ro.build.flavor property."
  echo "ro.random.prop=foo" > "${prop_file}"
  if flavor_prop=$(android_get_build_flavor_prop "${prop_file}"); then
    : $(( NUM_TEST_FAILURES += 1 ))
    echo "ERROR: Failed to detect missing ro.build.flavor property."
  else
    echo "PASS: Detected missing ro.build.flavor property."
  fi
  rm "${prop_file}"
}

choose_signing_keyset_test_helper() {
  local flavor_prop="$1"
  local expected_keyset="$2"
  local keyset=""

  keyset=$(android_choose_signing_keyset "${flavor_prop}")
  if [[ "${keyset}" != "${expected_keyset}" ]]; then
    : $(( NUM_TEST_FAILURES += 1 ))
    echo "ERROR: Incorrect keyset '${keyset}' returned instead of \
'${expected_keyset}'."
  else
    echo "PASS: Correct keyset '${keyset}' returned."
  fi
}

choose_signing_keyset_test_invalid_flavors() {
  local flavor="$1"

  echo "TEST: Detect invalid build flavor '${flavor}'."
  if android_choose_signing_keyset "${flavor}"; then
    : $(( NUM_TEST_FAILURES += 1 ))
    echo "ERROR: Failed to detect invalid build flavor '${flavor}'."
  else
    echo "PASS: Detected invalid build flavor '${flavor}'."
  fi
}

test_android_choose_signing_keyset() {
  echo "TEST: Keyset for aosp_cheets build."
  choose_signing_keyset_test_helper "aosp_cheets_x86-userdebug" "aosp"
  echo "TEST: Keyset for sdk_google_cheets build."
  choose_signing_keyset_test_helper "sdk_google_cheets_x86-userdebug" "cheets"
  echo "TEST: Keyset for cheets_x86 build."
  choose_signing_keyset_test_helper "cheets_x86-user" "cheets"
  echo "TEST: Keyset for cheets_arm build."
  choose_signing_keyset_test_helper "cheets_arm-user" "cheets"
  echo "TEST: Keyset for cheets_x86_64 build."
  choose_signing_keyset_test_helper "cheets_x86_64-user" "cheets"
  echo "TEST: Keyset for userdebug build."
  choose_signing_keyset_test_helper "cheets_x86-userdebug" "cheets"

  choose_signing_keyset_test_invalid_flavors "aosp"
  choose_signing_keyset_test_invalid_flavors "cheets"
  choose_signing_keyset_test_invalid_flavors ""
  choose_signing_keyset_test_invalid_flavors " "
}

main() {
  if [[ $# -ne 0 ]]; then
    echo "FAIL: unexpected arguments '$@'."
    return 1
  fi

  BUILD=$(mktemp -d)
  echo "Setting temporary build directory as '${BUILD}'."

  test_android_choose_key_invalid_keyset
  test_android_choose_key
  test_android_get_build_flavor_prop
  test_android_choose_signing_keyset

  echo "Deleting temporary build directory '${BUILD}'."
  rmdir "${BUILD}"

  if [[ ${NUM_TEST_FAILURES} -gt 0 ]]; then
    echo "FAIL: found ${NUM_TEST_FAILURES} failed :(."
    return 1
  fi
  echo "PASS: all tests passed :)."
  return 0
}

# Global incremented by each test when they fail.
NUM_TEST_FAILURES=0
main "$@"
