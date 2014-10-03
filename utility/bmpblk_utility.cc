// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Utility for manipulating firmware screen block (BMPBLOCK) in GBB.
//

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <lzma.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "bmpblk_utility.h"
#include "image_types.h"
#include "vboot_api.h"

extern "C" {
#include "eficompress.h"
}


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

  BmpBlockUtil::BmpBlockUtil(bool debug) {
    major_version_ = BMPBLOCK_MAJOR_VERSION;
    minor_version_ = BMPBLOCK_MINOR_VERSION;
    config_.config_filename.clear();
    memset(&config_.header, '\0', BMPBLOCK_SIGNATURE_SIZE);
    config_.images_map.clear();
    config_.screens_map.clear();
    config_.localizations.clear();
    bmpblock_.clear();
    set_compression_ = false;
    compression_ = COMPRESS_NONE;
    debug_ = debug;
    render_hwid_ = true;
    support_font_ = true;
    got_font_ = false;
    got_rtol_font_ = false;
  }

  BmpBlockUtil::~BmpBlockUtil() {
  }

  void BmpBlockUtil::force_compression(uint32_t compression) {
    compression_ = compression;
    set_compression_ = true;
  }

  void BmpBlockUtil::load_from_config(const char *filename) {
    load_yaml_config(filename);
    fill_bmpblock_header();
    load_all_image_files();
  }

  void BmpBlockUtil::load_yaml_config(const char *filename) {
    yaml_parser_t parser;

    config_.config_filename = filename;
    config_.images_map.clear();
    config_.screens_map.clear();
    config_.localizations.clear();
    config_.locale_names.clear();

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


    // TODO: Check the yaml file for self-consistency. Warn on any problems.
    // All images should be used somewhere in the screens.
    // All images referenced in the screens should be defined.
    // All screens should be used somewhere in the localizations.
    // All screens referenced in the localizations should be defined.
    // The number of localizations should match the number of locale_index

    if (debug_) {
      printf("%ld image_names\n", config_.image_names.size());
      for (unsigned int i = 0; i < config_.image_names.size(); ++i) {
        printf(" %d: \"%s\"\n", i, config_.image_names[i].c_str());
      }
      printf("%ld images_map\n", config_.images_map.size());
      for (StrImageConfigMap::iterator it = config_.images_map.begin();
           it != config_.images_map.end();
           ++it) {
        printf("  \"%s\": filename=\"%s\" offset=0x%x tag=%d fmt=%d\n",
               it->first.c_str(),
               it->second.filename.c_str(),
               it->second.offset,
               it->second.data.tag,
               it->second.data.format);
      }
      printf("%ld screens_map\n", config_.screens_map.size());
      for (StrScreenConfigMap::iterator it = config_.screens_map.begin();
           it != config_.screens_map.end();
           ++it) {
        printf("  \"%s\":\n", it->first.c_str());
        for (int k=0; k<MAX_IMAGE_IN_LAYOUT; k++) {
          printf("    %d: \"%s\" (%d,%d) ofs=0x%x\n",
                 k,
                 it->second.image_names[k].c_str(),
                 it->second.data.images[k].x,
                 it->second.data.images[k].y,
                 it->second.data.images[k].image_info_offset);
        }
      }
    }
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
        } else if (keyword == "compression") {
          parse_compression(parser);
        } else if (keyword == "images") {
          parse_images(parser);
        } else if (keyword == "screens") {
          parse_screens(parser);
        } else if (keyword == "localizations") {
          parse_localizations(parser);
        } else if (keyword == "locale_index") {
          parse_locale_index(parser);
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
    string gotversion = (char*)event.data.scalar.value;
    if (gotversion != "2.0") {
      error("Unsupported version specified in config file (%s)\n",
            gotversion.c_str());
    }
    yaml_event_delete(&event);
  }

  void BmpBlockUtil::parse_compression(yaml_parser_t *parser) {
    yaml_event_t event;
    yaml_parser_parse(parser, &event);
    if (event.type != YAML_SCALAR_EVENT) {
      error("Syntax error in parsing bmpblock.\n");
    }
    char *comp_str = (char *)event.data.scalar.value;
    char *e = 0;
    uint32_t comp = (uint32_t)strtoul(comp_str, &e, 0);
    if (!*comp_str || (e && *e) || comp >= MAX_COMPRESS) {
      error("Invalid compression specified in config file (%d)\n", comp);
    }
    if (!set_compression_) {
      compression_ = comp;
    }
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
        config_.image_names.push_back(image_name);
        config_.images_map[image_name] = ImageConfig();
        config_.images_map[image_name].filename = image_filename;
        if (image_name == RENDER_HWID) {
          got_font_ = true;
        }
        if (image_name == RENDER_HWID_RTOL) {
          got_rtol_font_ = true;
        }
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
          // Detect the special case where we're rendering the HWID string
          // instead of displaying a bitmap.  The image name may not
          // exist in the list of images (v1.1), but we will still need an
          // ImageInfo struct to remember where to draw the text.
          // Note that v1.2 requires that the image name DOES exist, because
          // the corresponding file is used to hold the font glpyhs.
          if (render_hwid_) {
            if (screen.image_names[index1] == RENDER_HWID) {
              config_.images_map[RENDER_HWID].data.tag = TAG_HWID;
              if (support_font_ && !got_font_)
                error("Font required in 'image:' section for %s\n",
                      RENDER_HWID);
            } else if (screen.image_names[index1] == RENDER_HWID_RTOL) {
              config_.images_map[RENDER_HWID_RTOL].data.tag = TAG_HWID_RTOL;
              if (support_font_ && !got_rtol_font_)
                error("Font required in 'image:' section for %s\n",
                      RENDER_HWID_RTOL);
            }
          }
          break;
        default:
          error("Syntax error in parsing layout\n");
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

  void BmpBlockUtil::parse_locale_index(yaml_parser_t *parser) {
    yaml_event_t event;
    expect_event(parser, YAML_SEQUENCE_START_EVENT);
    for (;;) {
      yaml_parser_parse(parser, &event);
      switch (event.type) {
      case YAML_SCALAR_EVENT:
        config_.locale_names.append((char*)event.data.scalar.value);
        config_.locale_names.append(1, (char)'\0'); // '\0' to delimit
        break;
      case YAML_SEQUENCE_END_EVENT:
        yaml_event_delete(&event);
        config_.locale_names.append(1, (char)'\0'); // double '\0' to terminate
        return;
      default:
        error("Syntax error in parsing localizations.\n");
      }
    }
  }

  void BmpBlockUtil::load_all_image_files() {
    for (unsigned int i = 0; i < config_.image_names.size(); i++) {
      StrImageConfigMap::iterator it =
        config_.images_map.find(config_.image_names[i]);
      if (debug_) {
        printf("loading image \"%s\" from \"%s\"\n",
               config_.image_names[i].c_str(),
               it->second.filename.c_str());
      }
      const string &content = read_image_file(it->second.filename.c_str());
      it->second.raw_content = content;
      it->second.data.original_size = content.size();
      it->second.data.format =
        identify_image_type(content.c_str(),
                            (uint32_t)content.size(), &it->second.data);
      if (FORMAT_INVALID == it->second.data.format) {
        error("Unsupported image format in %s\n", it->second.filename.c_str());
      }
      switch(compression_) {
      case COMPRESS_NONE:
        it->second.data.compression = compression_;
        it->second.compressed_content = content;
        it->second.data.compressed_size = content.size();
        break;
      case COMPRESS_EFIv1:
      {
        // The content will always compress smaller (so sez the docs).
        uint32_t tmpsize = content.size();
        uint8_t *tmpbuf = (uint8_t *)malloc(tmpsize);
        // The size of the compressed content is also returned.
        if (EFI_SUCCESS != EfiCompress((uint8_t *)content.c_str(), tmpsize,
                                       tmpbuf, &tmpsize)) {
          error("Unable to compress!\n");
        }
        it->second.data.compression = compression_;
        it->second.compressed_content.assign((const char *)tmpbuf, tmpsize);
        it->second.data.compressed_size = tmpsize;
        free(tmpbuf);
      }
      break;
      case COMPRESS_LZMA1:
      {
        // Calculate the worst case of buffer size.
        uint32_t tmpsize = lzma_stream_buffer_bound(content.size());
        uint8_t *tmpbuf = (uint8_t *)malloc(tmpsize);
        lzma_stream stream = LZMA_STREAM_INIT;
        lzma_options_lzma options;
        lzma_ret result;

        lzma_lzma_preset(&options, 9);
        result = lzma_alone_encoder(&stream, &options);
        if (result != LZMA_OK) {
          error("Unable to initialize easy encoder (error: %d)!\n", result);
        }

        stream.next_in = (uint8_t *)content.data();
        stream.avail_in = content.size();
        stream.next_out = tmpbuf;
        stream.avail_out = tmpsize;
        result = lzma_code(&stream, LZMA_FINISH);
        if (result != LZMA_STREAM_END) {
          error("Unable to encode data (error: %d)!\n", result);
        }

        it->second.data.compression = compression_;
        it->second.compressed_content.assign((const char *)tmpbuf,
                                             tmpsize - stream.avail_out);
        it->second.data.compressed_size = tmpsize - stream.avail_out;
        lzma_end(&stream);
        free(tmpbuf);
      }
      break;
      default:
        error("Unsupported compression method attempted.\n");
      }
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

  void BmpBlockUtil::fill_bmpblock_header() {
    memset(&config_.header, '\0', sizeof(config_.header));
    memcpy(&config_.header.signature, BMPBLOCK_SIGNATURE,
           BMPBLOCK_SIGNATURE_SIZE);
    config_.header.major_version = major_version_;
    config_.header.minor_version = minor_version_;
    config_.header.number_of_localizations = config_.localizations.size();
    config_.header.number_of_screenlayouts = config_.localizations[0].size();
    // NOTE: this is part of the yaml consistency check
    for (unsigned int i = 1; i < config_.localizations.size(); ++i) {
      assert(config_.header.number_of_screenlayouts ==
             config_.localizations[i].size());
    }
    config_.header.number_of_imageinfos = config_.images_map.size();
    config_.header.locale_string_offset = 0; // Filled by pack_bmpblock()
  }

  void BmpBlockUtil::pack_bmpblock() {
    bmpblock_.clear();

    /* Compute the ImageInfo offsets from start of BMPBLOCK. */
    uint32_t current_offset = sizeof(BmpBlockHeader) +
      sizeof(ScreenLayout) * (config_.header.number_of_localizations *
                              config_.header.number_of_screenlayouts);
    for (StrImageConfigMap::iterator it = config_.images_map.begin();
         it != config_.images_map.end();
         ++it) {
      it->second.offset = current_offset;
      if (debug_)
        printf("  \"%s\": filename=\"%s\" offset=0x%x tag=%d fmt=%d\n",
               it->first.c_str(),
               it->second.filename.c_str(),
               it->second.offset,
               it->second.data.tag,
               it->second.data.format);
      current_offset += sizeof(ImageInfo) +
        it->second.data.compressed_size;
      /* Make it 4-byte aligned. */
      if ((current_offset & 3) > 0) {
        current_offset = (current_offset & ~3) + 4;
      }
    }
    /* And leave room for the locale_index string */
    if (config_.locale_names.size()) {
      config_.header.locale_string_offset = current_offset;
      current_offset += config_.locale_names.size();
    }

    bmpblock_.resize(current_offset);

    /* Fill BmpBlockHeader struct. */
    string::iterator current_filled = bmpblock_.begin();
    std::copy(reinterpret_cast<char*>(&config_.header),
              reinterpret_cast<char*>(&config_.header + 1),
              current_filled);
    current_filled += sizeof(config_.header);
    current_offset = sizeof(config_.header);

    /* Fill all ScreenLayout structs. */
    for (unsigned int i = 0; i < config_.localizations.size(); ++i) {
      for (unsigned int j = 0; j < config_.localizations[i].size(); ++j) {
        ScreenConfig &screen = config_.screens_map[config_.localizations[i][j]];
        for (unsigned int k = 0;
             k < MAX_IMAGE_IN_LAYOUT && !screen.image_names[k].empty();
             ++k) {
          if (config_.images_map.find(screen.image_names[k]) ==
              config_.images_map.end()) {
            error("Invalid image name \"%s\"\n", screen.image_names[k].c_str());
          }
          if (debug_)
            printf("i=%d j=%d k=%d=\"%s\" (%d,%d) ofs=%x\n", i,j,k,
                   screen.image_names[k].c_str(),
                   screen.data.images[k].x, screen.data.images[k].y,
                   config_.images_map[screen.image_names[k]].offset
              );
          screen.data.images[k].image_info_offset =
            config_.images_map[screen.image_names[k]].offset;
        }
        std::copy(reinterpret_cast<char*>(&screen.data),
                  reinterpret_cast<char*>(&screen.data + 1),
                  current_filled);
        current_filled += sizeof(screen.data);
        if (debug_)
          printf("S: current offset is 0x%08x\n", current_offset);
        current_offset += sizeof(screen.data);
      }
    }

    /* Fill all ImageInfo structs and image contents. */
    for (StrImageConfigMap::iterator it = config_.images_map.begin();
         it != config_.images_map.end();
         ++it) {
      current_filled = bmpblock_.begin() + it->second.offset;
      current_offset = it->second.offset;
      if (debug_)
        printf("I0: current offset is 0x%08x\n", current_offset);
      std::copy(reinterpret_cast<char*>(&it->second.data),
                reinterpret_cast<char*>(&it->second.data + 1),
                current_filled);
      current_filled += sizeof(it->second.data);
      current_offset += sizeof(it->second.data);
      if (debug_)
        printf("I1: current offset is 0x%08x (len %ld)\n",
               current_offset, it->second.compressed_content.length());
      std::copy(it->second.compressed_content.begin(),
                it->second.compressed_content.end(),
                current_filled);
    }

    /* Fill in locale_names. */
    if (config_.header.locale_string_offset) {
      current_offset = config_.header.locale_string_offset;
      current_filled = bmpblock_.begin() + current_offset;
      if (debug_)
        printf("locale_names: offset 0x%08x (len %ld)\n",
               current_offset, config_.locale_names.size());
      std::copy(config_.locale_names.begin(),
                config_.locale_names.end(),
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

#ifndef FOR_LIBRARY

  //////////////////////////////////////////////////////////////////////////////
  // Command line utilities.

  extern "C" {
#include "bmpblk_util.h"
  }

  using vboot_reference::BmpBlockUtil;

  // utility function: provide usage of this utility and exit.
  static void usagehelp_exit(const char *prog_name) {
    printf(
      "\n"
      "To create a new BMPBLOCK file using config from YAML file:\n"
      "\n"
      "  %s [-z NUM] -c YAML BMPBLOCK\n"
      "\n"
      "    -z NUM  = compression algorithm to use\n"
      "              0 = none\n"
      "              1 = EFIv1\n"
      "              2 = LZMA1\n"
      "\n", prog_name);
    printf(
      "To display the contents of a BMPBLOCK:\n"
      "\n"
      "  %s [-y] BMPBLOCK\n"
      "\n"
      "    -y  = display as yaml\n"
      "\n", prog_name);
    printf(
      "To unpack a BMPBLOCK file:\n"
      "\n"
      "  %s -x [-d DIR] [-f] BMPBLOCK\n"
      "\n"
      "    -d DIR  = directory to use (default '.')\n"
      "    -f      = force overwriting existing files\n"
      "\n", prog_name);
    exit(1);
  }

  ///////////////////////////////////////////////////////////////////////
  // main

  int main(int argc, char *argv[]) {

    const char *prog_name = strrchr(argv[0], '/');
    if (prog_name)
      prog_name++;
    else
      prog_name = argv[0];

    int overwrite = 0, extract_mode = 0;
    int compression = 0;
    int set_compression = 0;
    const char *config_fn = 0, *bmpblock_fn = 0, *extract_dir = ".";
    int show_as_yaml = 0;
    bool debug = false;

    int opt;
    opterr = 0;                           // quiet
    int errorcnt = 0;
    char *e = 0;
    while ((opt = getopt(argc, argv, ":c:xz:fd:yD")) != -1) {
      switch (opt) {
      case 'c':
        config_fn = optarg;
        break;
      case 'x':
        extract_mode = 1;
        break;
      case 'y':
        show_as_yaml = 1;
        break;
      case 'z':
        compression = (int)strtoul(optarg, &e, 0);
        if (!*optarg || (e && *e)) {
          fprintf(stderr, "%s: invalid argument to -%c: \"%s\"\n",
                  prog_name, opt, optarg);
          errorcnt++;
        }
        if (compression >= MAX_COMPRESS) {
          fprintf(stderr, "%s: compression type must be less than %d\n",
                  prog_name, MAX_COMPRESS);
          errorcnt++;
        }
        set_compression = 1;
        break;
      case 'f':
        overwrite = 1;
        break;
      case 'd':
        extract_dir= optarg;
        break;
      case 'D':
        debug = true;
        break;
      case ':':
        fprintf(stderr, "%s: missing argument to -%c\n",
                prog_name, optopt);
        errorcnt++;
        break;
      default:
        fprintf(stderr, "%s: unrecognized switch: -%c\n",
                prog_name, optopt);
        errorcnt++;
        break;
      }
    }
    argc -= optind;
    argv += optind;

    if (argc >= 1) {
      bmpblock_fn = argv[0];
    } else {
      fprintf(stderr, "%s: missing BMPBLOCK name\n", prog_name);
      errorcnt++;
    }

    if (errorcnt)
      usagehelp_exit(prog_name);

    BmpBlockUtil util(debug);

    if (config_fn) {
      if (set_compression)
        util.force_compression(compression);
      util.load_from_config(config_fn);
      util.pack_bmpblock();
      util.write_to_bmpblock(bmpblock_fn);
    }

    else if (extract_mode) {
      return dump_bmpblock(bmpblock_fn, 1, extract_dir, overwrite);
    } else {
      return dump_bmpblock(bmpblock_fn, show_as_yaml, 0, 0);
    }

    return 0;
  }

#endif // FOR_LIBRARY
