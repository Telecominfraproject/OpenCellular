// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#ifndef VBOOT_REFERENCE_BMPBLK_UTILITY_H_
#define VBOOT_REFERENCE_BMPBLK_UTILITY_H_

#include "bmpblk_header.h"

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
  StrImageConfigMap images_map;
  StrScreenConfigMap screens_map;
  vector<vector<string> > localizations;
} BmpBlockConfig;

class BmpBlockUtil {
 public:
  BmpBlockUtil();
  ~BmpBlockUtil();

  /* Load all the images and related infomations according to a config file. */
  void load_from_config(const char *filename);

  /* Compress all the images using a given comression method. */
  void compress_all_images(const Compression compress);

  /* Contruct the bmpblock. */
  void pack_bmpblock();

  /* Write the bmpblock to a file */
  void write_to_bmpblock(const char *filename);

 private:
  /* Clear all internal data. */
  void initialize();

  /* Elemental function called from load_from_config.
   * Load the config file (yaml format) and parse it. */
  void load_yaml_config(const char *filename);

  /* Elemental function called from load_from_config.
   * Load all image files into the internal variables. */
  void load_all_image_files();

  /* Elemental function called from load_from_config.
   * Contruct all ImageInfo structs. */
  void fill_all_image_infos();

  /* Elemental function called from load_from_config.
   * Contruct the BmpBlockHeader struct. */
  void fill_bmpblock_header();

  /* Helper functions for parsing a YAML config file. */
  void expect_event(yaml_parser_t *parser, const yaml_event_type_e type);
  void parse_config(yaml_parser_t *parser);
  void parse_first_layer(yaml_parser_t *parser);
  void parse_bmpblock(yaml_parser_t *parser);
  void parse_images(yaml_parser_t *parser);
  void parse_layout(yaml_parser_t *parser, ScreenConfig &screen);
  void parse_screens(yaml_parser_t *parser);
  void parse_localizations(yaml_parser_t *parser);

  /* Useful functions */
  const string read_image_file(const char *filename);
  ImageFormat get_image_format(const string content);
  uint32_t get_bmp_image_width(const string content);
  uint32_t get_bmp_image_height(const string content);

  /* Internal variable for storing the config of BmpBlock. */
  BmpBlockConfig config_;

  /* Internal variable for storing the content of BmpBlock. */
  string bmpblock_;
};

}  // namespace vboot_reference

#endif  // VBOOT_REFERENCE_BMPBLK_UTILITY_H_
