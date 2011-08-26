// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#ifndef VBOOT_REFERENCE_GBB_UTILITY_H_
#define VBOOT_REFERENCE_GBB_UTILITY_H_

#include <string>
#include <vector>
#include "gbb_header.h"

namespace vboot_reference {

class GoogleBinaryBlockUtil {
 public:
  // enumerate of available data fields
  enum PROPINDEX {
    PROP_FLAGS = -1,// flags (virtual property)
    PROP_HWID,      // hardware id
    PROP_ROOTKEY,   // root key
    PROP_BMPFV,     // bitmap FV
    PROP_RCVKEY,    // recovery key
    PROP_RANGE,     // indicator of valid property range
  };

  GoogleBinaryBlockUtil();
  ~GoogleBinaryBlockUtil();

  // load GBB from a BIOS image file.
  // return true if a valid GBB was retrieved.
  bool load_from_file(const char *filename);

  // save loaded (and modified) GBB with BIOS image to new file
  // return true on success.
  bool save_to_file(const char *filename);

  // create a new GBB blob by providing a list of reserved data size for each
  // properties, following the order described in GoogleBinaryBlockHeader.
  // return true on success.
  bool create_new(const std::vector<uint32_t> &create_param);

  // retrieve the value of GBB header flags.
  // return the flags value.
  uint32_t get_flags() const;

  // overwrite GBB header flags.
  // return true on success.
  bool set_flags(const uint32_t flags);

  // retrieve the value of a property from GBB data.
  // return the property value.
  std::string get_property(PROPINDEX i) const;

  // overwrite a property in GBB data.
  // return true on success.
  bool set_property(PROPINDEX i, const std::string &value);

  // get a readable name by a property index.
  // return the name for valid properties, otherwise unexpected empty string.
  std::string get_property_name(PROPINDEX i) const;

  // quick getters and setters of known properties in GBB
  bool set_hwid(const char *hwid);      // NOTE: hwid is NUL-terminated.
  bool set_rootkey(const std::string &value);
  bool set_bmpfv(const std::string &value);
  bool set_recovery_key(const std::string &value);
  std::string get_hwid() const { return get_property(PROP_HWID); }
  std::string get_rootkey() const { return get_property(PROP_ROOTKEY); }
  std::string get_bmpfv() const { return get_property(PROP_BMPFV); }
  std::string get_recovery_key() const { return get_property(PROP_RCVKEY); }

 private:
  // clear all cached data and initialize to original state
  void initialize();

  // search and count for GBB signatures in loaded image.
  // return the number of signatures found.
  int search_header_signatures(const std::string &image, long *poffset) const;

  // load and check header structure from image by given offset.
  // return true if a valid GBB header is loaded into *phdr.
  bool load_gbb_header(const std::string &image, long offset,
                       GoogleBinaryBlockHeader *phdr) const;

  // find the size, offset, and name information for given property.
  // return true if the offset and size are assign to *poffset and *psize;
  // if pname is not NULL, *pname will hold a pointer to a readable name.
  // return false if the property index is invalid.
  bool find_property(PROPINDEX i, uint32_t *poffset, uint32_t *psize,
                     const char **pname) const;

  GoogleBinaryBlockHeader header_;      // copy of GBB header from image
  std::string file_content_;            // complete image file content
  long header_offset_;                  // offset to GBB header in file_content_
  bool is_valid_gbb;                    // if we are holding a valid GBB

  bool verbose;                         // provide verbose messages
};

}  // namespace vboot_reference

#endif  // VBOOT_REFERENCE_GBB_UTILITY_H_
