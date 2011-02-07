// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Utility for manipulating firmware screen block (BMPBLOCK) in GBB.
//

#include "bmpblk_utility.h"

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

/* The offsets of width and height fields in a BMP file.
 * See http://en.wikipedia.org/wiki/BMP_file_format */
#define BMP_WIDTH_OFFSET   18
#define BMP_HEIGHT_OFFSET  22

static void error(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  fprintf(stderr, "ERROR: ");
  vfprintf(stderr, format, ap);
  va_end(ap);
  exit(1);
}

///////////////////////////////////////////////////////////////////////
// BmpBlock Utility implementation

namespace vboot_reference {

BmpBlockUtil::BmpBlockUtil() {
  initialize();
}

BmpBlockUtil::~BmpBlockUtil() {
}

void BmpBlockUtil::initialize() {
  config_.config_filename.clear();
  memset(&config_.header, '\0', BMPBLOCK_SIGNATURE_SIZE);
  config_.images_map.clear();
  config_.screens_map.clear();
  config_.localizations.clear();
  bmpblock_.clear();
}

void BmpBlockUtil::load_from_config(const char *filename) {
  load_yaml_config(filename);
  fill_bmpblock_header();
  load_all_image_files();
  fill_all_image_infos();
}

void BmpBlockUtil::load_yaml_config(const char *filename) {
  yaml_parser_t parser;

  config_.config_filename = filename;
  config_.images_map.clear();
  config_.screens_map.clear();
  config_.localizations.clear();

  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    perror(filename);
    exit(errno);
  }

  yaml_parser_initialize(&parser);
  yaml_parser_set_input_file(&parser, fp);
  parse_config(&parser);
  yaml_parser_delete(&parser);
  fclose(fp);
}

void BmpBlockUtil::expect_event(yaml_parser_t *parser,
                                const yaml_event_type_e type) {
  yaml_event_t event;
  yaml_parser_parse(parser, &event);
  if (event.type != type) {
    error("Syntax error.\n");
  }
  yaml_event_delete(&event);
}

void BmpBlockUtil::parse_config(yaml_parser_t *parser) {
  expect_event(parser, YAML_STREAM_START_EVENT);
  expect_event(parser, YAML_DOCUMENT_START_EVENT);
  parse_first_layer(parser);
  expect_event(parser, YAML_DOCUMENT_END_EVENT);
  expect_event(parser, YAML_STREAM_END_EVENT);
}

void BmpBlockUtil::parse_first_layer(yaml_parser_t *parser) {
  yaml_event_t event;
  string keyword;
  expect_event(parser, YAML_MAPPING_START_EVENT);
  for (;;) {
    yaml_parser_parse(parser, &event);
    switch (event.type) {
      case YAML_SCALAR_EVENT:
        keyword = (char*)event.data.scalar.value;
        if (keyword == "bmpblock") {
          parse_bmpblock(parser);
        } else if (keyword == "images") {
          parse_images(parser);
        } else if (keyword == "screens") {
          parse_screens(parser);
        } else if (keyword == "localizations") {
          parse_localizations(parser);
        }
        break;
      case YAML_MAPPING_END_EVENT:
        yaml_event_delete(&event);
        return;
      default:
        error("Syntax error in parsing config file.\n");
    }
    yaml_event_delete(&event);
  }
}

void BmpBlockUtil::parse_bmpblock(yaml_parser_t *parser) {
  yaml_event_t event;
  yaml_parser_parse(parser, &event);
  if (event.type != YAML_SCALAR_EVENT) {
    error("Syntax error in parsing bmpblock.\n");
  }
  config_.header.major_version = atoi((char*)event.data.scalar.value);
  config_.header.minor_version = atoi(
      strchr((char*)event.data.scalar.value, '.') + 1);
  yaml_event_delete(&event);
}

void BmpBlockUtil::parse_images(yaml_parser_t *parser) {
  yaml_event_t event;
  string image_name, image_filename;
  expect_event(parser, YAML_MAPPING_START_EVENT);
  for (;;) {
    yaml_parser_parse(parser, &event);
    switch (event.type) {
      case YAML_SCALAR_EVENT:
        image_name = (char*)event.data.scalar.value;
        yaml_event_delete(&event);
        yaml_parser_parse(parser, &event);
        if (event.type != YAML_SCALAR_EVENT) {
          error("Syntax error in parsing images.\n");
        }
        image_filename = (char*)event.data.scalar.value;
        config_.images_map[image_name] = ImageConfig();
        config_.images_map[image_name].filename = image_filename;
        break;
      case YAML_MAPPING_END_EVENT:
        yaml_event_delete(&event);
        return;
      default:
        error("Syntax error in parsing images.\n");
    }
    yaml_event_delete(&event);
  }
}

void BmpBlockUtil::parse_layout(yaml_parser_t *parser, ScreenConfig &screen) {
  yaml_event_t event;
  string screen_name;
  int depth = 0, index1 = 0, index2 = 0;
  expect_event(parser, YAML_SEQUENCE_START_EVENT);
  for (;;) {
    yaml_parser_parse(parser, &event);
    switch (event.type) {
      case YAML_SEQUENCE_START_EVENT:
        depth++;
        break;
      case YAML_SCALAR_EVENT:
        switch (index2) {
          case 0:
            screen.data.images[index1].x = atoi((char*)event.data.scalar.value);
            break;
          case 1:
            screen.data.images[index1].y = atoi((char*)event.data.scalar.value);
            break;
          case 2:
            screen.image_names[index1] = (char*)event.data.scalar.value;
            break;
          default:
            error("Syntax error in parsing layout.\n");
        }
        index2++;
        break;
      case YAML_SEQUENCE_END_EVENT:
        if (depth == 1) {
          index1++;
          index2 = 0;
        } else if (depth == 0) {
          yaml_event_delete(&event);
          return;
        }
        depth--;
        break;
      default:
        error("Syntax error in paring layout.\n");
    }
    yaml_event_delete(&event);
  }
}

void BmpBlockUtil::parse_screens(yaml_parser_t *parser) {
  yaml_event_t event;
  string screen_name;
  expect_event(parser, YAML_MAPPING_START_EVENT);
  for (;;) {
    yaml_parser_parse(parser, &event);
    switch (event.type) {
      case YAML_SCALAR_EVENT:
        screen_name = (char*)event.data.scalar.value;
        config_.screens_map[screen_name] = ScreenConfig();
        parse_layout(parser, config_.screens_map[screen_name]);
        break;
      case YAML_MAPPING_END_EVENT:
        yaml_event_delete(&event);
        return;
      default:
        error("Syntax error in parsing screens.\n");
    }
    yaml_event_delete(&event);
  }
}

void BmpBlockUtil::parse_localizations(yaml_parser_t *parser) {
  yaml_event_t event;
  int depth = 0, index = 0;
  expect_event(parser, YAML_SEQUENCE_START_EVENT);
  for (;;) {
    yaml_parser_parse(parser, &event);
    switch (event.type) {
      case YAML_SEQUENCE_START_EVENT:
        config_.localizations.push_back(vector<string>());
        depth++;
        break;
      case YAML_SCALAR_EVENT:
        config_.localizations[index].push_back((char*)event.data.scalar.value);
        break;
      case YAML_SEQUENCE_END_EVENT:
        if (depth == 1) {
          index++;
        } else if (depth == 0) {
          yaml_event_delete(&event);
          return;
        }
        depth--;
        break;
      default:
        error("Syntax error in parsing localizations.\n");
    }
    yaml_event_delete(&event);
  }
}

void BmpBlockUtil::load_all_image_files() {
  for (StrImageConfigMap::iterator it = config_.images_map.begin();
       it != config_.images_map.end();
       ++it) {
    const string &content = read_image_file(it->second.filename.c_str());
    it->second.raw_content = content;
    it->second.data.original_size = content.size();
    /* Use no compression as default */
    it->second.data.compression = COMPRESS_NONE;
    it->second.compressed_content = content;
    it->second.data.compressed_size = content.size();
  }
}

const string BmpBlockUtil::read_image_file(const char *filename) {
  string content;
  vector<char> buffer;

  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    perror(filename);
    exit(errno);
  }

  if (fseek(fp, 0, SEEK_END) == 0) {
    buffer.resize(ftell(fp));
    rewind(fp);
  }

  if (!buffer.empty()) {
    if(fread(&buffer[0], buffer.size(), 1, fp) != 1) {
      perror(filename);
      buffer.clear();
    } else {
      content.assign(buffer.begin(), buffer.end());
    }
  }

  fclose(fp);
  return content;
}

ImageFormat BmpBlockUtil::get_image_format(const string content) {
  if (content[0] == 'B' && content[1] == 'M')
    return FORMAT_BMP;
  else
    return FORMAT_INVALID;
}

uint32_t BmpBlockUtil::get_bmp_image_width(const string content) {
  const char *start = content.c_str();
  uint32_t width = *(uint32_t*)(start + BMP_WIDTH_OFFSET);
  /* Do a rough verification. */
  assert(width > 0 && width < 1600);
  return width;
}

uint32_t BmpBlockUtil::get_bmp_image_height(const string content) {
  const char *start = content.c_str();
  uint32_t height = *(uint32_t*)(start + BMP_HEIGHT_OFFSET);
  /* Do a rough verification. */
  assert(height > 0 && height < 1000);
  return height;
}

void BmpBlockUtil::fill_all_image_infos() {
  for (StrImageConfigMap::iterator it = config_.images_map.begin();
       it != config_.images_map.end();
       ++it) {
    it->second.data.format = (uint32_t)get_image_format(it->second.raw_content);
    switch (it->second.data.format) {
      case FORMAT_BMP:
        it->second.data.width = get_bmp_image_width(it->second.raw_content);
        it->second.data.height = get_bmp_image_height(it->second.raw_content);
        break;
      default:
        error("Unsupported image format.\n");
    }
  }
}

void BmpBlockUtil::compress_all_images(const Compression compress) {
  switch (compress) {
    case COMPRESS_NONE:
      for (StrImageConfigMap::iterator it = config_.images_map.begin();
           it != config_.images_map.end();
           ++it) {
        it->second.data.compression = compress;
        it->second.compressed_content = it->second.raw_content;
        it->second.data.compressed_size = it->second.compressed_content.size();
      }
      break;
    default:
      error("Unsupported data compression.\n");
  }
}

void BmpBlockUtil::fill_bmpblock_header() {
  memset(&config_.header, '\0', sizeof(config_.header));
  memcpy(&config_.header.signature, BMPBLOCK_SIGNATURE,
         BMPBLOCK_SIGNATURE_SIZE);
  config_.header.number_of_localizations = config_.localizations.size();
  config_.header.number_of_screenlayouts = config_.localizations[0].size();
  for (unsigned int i = 1; i < config_.localizations.size(); ++i) {
    assert(config_.header.number_of_screenlayouts ==
        config_.localizations[i].size());
  }
  config_.header.number_of_imageinfos = config_.images_map.size();
}

void BmpBlockUtil::pack_bmpblock() {
  bmpblock_.clear();

  /* Compute the ImageInfo offsets from start of BMPBLOCK. */
  uint32_t current_offset = sizeof(BmpBlockHeader) +
                            sizeof(ScreenLayout) * config_.images_map.size();
  for (StrImageConfigMap::iterator it = config_.images_map.begin();
       it != config_.images_map.end();
       ++it) {
    it->second.offset = current_offset;
    current_offset += sizeof(ImageInfo) + it->second.data.compressed_size;
    /* Make it 4-byte aligned. */
    if ((current_offset & 3) > 0) {
      current_offset = (current_offset & ~3) + 4;
    }
  }
  bmpblock_.resize(current_offset);

  /* Fill BmpBlockHeader struct. */
  string::iterator current_filled = bmpblock_.begin();
  std::copy(reinterpret_cast<char*>(&config_.header),
            reinterpret_cast<char*>(&config_.header + 1),
            current_filled);
  current_filled += sizeof(config_.header);

  /* Fill all ScreenLayout structs. */
  for (unsigned int i = 0; i < config_.localizations.size(); ++i) {
    for (unsigned int j = 0; j < config_.localizations[i].size(); ++j) {
      ScreenConfig &screen = config_.screens_map[config_.localizations[i][j]];
      for (unsigned int k = 0;
           k < MAX_IMAGE_IN_LAYOUT && !screen.image_names[k].empty();
           ++k) {
        screen.data.images[k].image_info_offset =
            config_.images_map[screen.image_names[k]].offset;
      }
      std::copy(reinterpret_cast<char*>(&screen.data),
                reinterpret_cast<char*>(&screen.data + 1),
                current_filled);
      current_filled += sizeof(screen.data);
    }
  }

  /* Fill all ImageInfo structs and image contents. */
  for (StrImageConfigMap::iterator it = config_.images_map.begin();
       it != config_.images_map.end();
       ++it) {
    current_filled = bmpblock_.begin() + it->second.offset;
    std::copy(reinterpret_cast<char*>(&it->second.data),
              reinterpret_cast<char*>(&it->second.data + 1),
              current_filled);
    current_filled += sizeof(it->second.data);
    std::copy(it->second.compressed_content.begin(),
              it->second.compressed_content.end(),
              current_filled);
  }
}

void BmpBlockUtil::write_to_bmpblock(const char *filename) {
  assert(!bmpblock_.empty());

  FILE *fp = fopen(filename, "wb");
  if (!fp) {
    perror(filename);
    exit(errno);
  }

  int r = fwrite(bmpblock_.c_str(), bmpblock_.size(), 1, fp);
  fclose(fp);
  if (r != 1) {
    perror(filename);
    exit(errno);
  }
}

}  // namespace vboot_reference

#ifdef WITH_UTIL_MAIN

///////////////////////////////////////////////////////////////////////
// Command line utilities

using vboot_reference::BmpBlockUtil;

// utility function: provide usage of this utility and exit.
static void usagehelp_exit(const char *prog_name) {
  printf(
    "Utility to manage firmware screen block (BMPBLOCK)\n"
    "Usage: %s -c|-l|-x [options] BMPBLOCK_FILE\n"
    "\n"
    "Main Operation Mode:\n"
    "  -c, --create   Create a new BMPBLOCK file. Should specify --config.\n"
    "  -l, --list     List the contents of a BMPBLOCK file.\n"
    "  -x, --extract  Extract embedded images and config file from a BMPBLOCK\n"
    "                 file.\n"
    "\n"
    "Other Options:\n"
    "  -C, --config=CONFIG_FILE    Config file describing screen layouts and\n"
    "                              embedded images. (default: bmpblk.cfg)\n"
    "\n"
    "Example:\n"
    "  %s --create --config=screens.cfg bmpblk.bin\n"
    , prog_name, prog_name);
  exit(1);
}

///////////////////////////////////////////////////////////////////////
// main

int main(int argc, char *argv[]) {
  const char *prog_name = argv[0];
  BmpBlockUtil util;

  struct BmpBlockUtilOptions {
    bool create_mode, list_mode, extract_mode;
    string config_fn, bmpblock_fn;
  } options;

  int longindex, opt;
  static struct option longopts[] = {
    {"create",  0, NULL, 'c'},
    {"list",    0, NULL, 'l'},
    {"extract", 0, NULL, 'x'},
    {"config",  1, NULL, 'C'},
    { NULL,     0, NULL,  0 },
  };

  while ((opt = getopt_long(argc, argv, "clxC:", longopts, &longindex)) >= 0) {
    switch (opt) {
      case 'c':
        options.create_mode = true;
        break;
      case 'l':
        options.list_mode = true;
        break;
      case 'x':
        options.extract_mode = true;
        break;
      case 'C':
        options.config_fn = optarg;
        break;
      default:
      case '?':
        usagehelp_exit(prog_name);
        break;
    }
  }
  argc -= optind;
  argv += optind;

  if (argc == 1) {
    options.bmpblock_fn = argv[0];
  } else {
    usagehelp_exit(prog_name);
  }

  if (options.create_mode) {
    util.load_from_config(options.config_fn.c_str());
    util.pack_bmpblock();
    util.write_to_bmpblock(options.bmpblock_fn.c_str());
    printf("The BMPBLOCK is sucessfully created in: %s.\n",
           options.bmpblock_fn.c_str());
  }

  if (options.list_mode) {
    /* TODO(waihong): Implement the list mode. */
    error("List mode hasn't been implemented yet.\n");
  }

  if (options.extract_mode) {
    /* TODO(waihong): Implement the extract mode. */
    error("Extract mode hasn't been implemented yet.\n");
  }

  return 0;
}

#endif  // WITH_UTIL_MAIN
