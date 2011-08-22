// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#ifndef VBOOT_REFERENCE_BMPBLK_UTILITY_H_
#define VBOOT_REFERENCE_BMPBLK_UTILITY_H_

#include "bmpblk_header.h"
#include "bmpblk_font.h"
#include "image_types.h"

#include <yaml.h>

#include <map>
#include <string>
#include <vector>

using std::map;
using std::string;
using std::vector;

namespace vboot_reference {

/* Internal struct for contructing ImageInfo. */
typedef struct ImageConfig {
  ImageInfo data;
  string filename;
  string raw_content;
  string compressed_content;
  uint32_t offset;
} ImageConfig;

/* Internal struct for contructing ScreenLayout. */
typedef struct ScreenConfig {
  ScreenLayout data;
  string image_names[MAX_IMAGE_IN_LAYOUT];
} ScreenConfig;

typedef map<string, ImageConfig> StrImageConfigMap;
typedef map<string, ScreenConfig> StrScreenConfigMap;

/* Internal struct for contructing the whole BmpBlock. */
typedef struct BmpBlockConfig {
  string config_filename;
  BmpBlockHeader header;
  vector<string> image_names;
  StrImageConfigMap images_map;
  StrScreenConfigMap screens_map;
  vector<vector<string> > localizations;
  string locale_names;
} BmpBlockConfig;

class BmpBlockUtil {
 public:
  BmpBlockUtil(bool debug);
  ~BmpBlockUtil();

  /* Load all the images and related infomations according to a config file. */
  void load_from_config(const char *filename);

  /* Contruct the bmpblock. */
  void pack_bmpblock();

  /* Write the bmpblock to a file */
  void write_to_bmpblock(const char *filename);

  /* What compression to use for the images */
  void force_compression(uint32_t compression);

 private:
  /* Elemental function called from load_from_config.
   * Load the config file (yaml format) and parse it. */
  void load_yaml_config(const char *filename);

  /* Elemental function called from load_from_config.
   * Load all image files into the internal variables. */
  void load_all_image_files();

  /* Elemental function called from load_from_config.
   * Contruct the BmpBlockHeader struct. */
  void fill_bmpblock_header();

  /* Helper functions for parsing a YAML config file. */
  void expect_event(yaml_parser_t *parser, const yaml_event_type_e type);
  void parse_config(yaml_parser_t *parser);
  void parse_first_layer(yaml_parser_t *parser);
  void parse_bmpblock(yaml_parser_t *parser);
  void parse_compression(yaml_parser_t *parser);
  void parse_images(yaml_parser_t *parser);
  void parse_layout(yaml_parser_t *parser, ScreenConfig &screen);
  void parse_screens(yaml_parser_t *parser);
  void parse_localizations(yaml_parser_t *parser);
  void parse_locale_index(yaml_parser_t *parser);

  /* Useful functions */
  const string read_image_file(const char *filename);

  /* Verbosity flags */
  bool debug_;

  /* Internal variable for string the BmpBlock version. */
  uint16_t major_version_;
  uint16_t minor_version_;

  /* Flags for version-specific features */
  bool render_hwid_;
  bool support_font_;
  bool got_font_;
  bool got_rtol_font_;

  /* Internal variable for storing the config of BmpBlock. */
  BmpBlockConfig config_;

  /* Internal variable for storing the content of BmpBlock. */
  string bmpblock_;

  /* Internal variables to determine whether or not to specify compression */
  bool set_compression_;                // true if we force it
  uint32_t compression_;                // what we force it to
};

}  // namespace vboot_reference

#endif  // VBOOT_REFERENCE_BMPBLK_UTILITY_H_
