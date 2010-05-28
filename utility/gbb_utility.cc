// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Utility for manipulating Google Binary Block (GBB)
//

#include "gbb_utility.h"

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>

#include <string>
#include <vector>
#include <algorithm>

using std::string;

///////////////////////////////////////////////////////////////////////
// Simple File Utilities

// utility function: read a non-empty file.
// return file content, or empty for any failure.
static string read_nonempty_file(const char *filename) {
  string file_content;
  std::vector<char> buffer;     // since image files are small, should be OK

  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    perror(filename);
    return file_content;
  }

  // prepare buffer on successful seek
  if (fseek(fp, 0, SEEK_END) == 0) {
    buffer.resize(ftell(fp));
    rewind(fp);
  }

  if (!buffer.empty()) {
    if (fread(&buffer[0], buffer.size(), 1, fp) != 1) {
      perror(filename);
      buffer.clear();  // discard buffer when read fail.
    } else {
      file_content.assign(buffer.begin(), buffer.end());
    }
  }

  fclose(fp);
  return file_content;
}

// utility function: write non-empty content to file.
// return true on success, otherwise false.
static bool write_nonempty_file(const char *filename, const string &content) {
  assert(!content.empty());

  FILE *fp = fopen(filename, "wb");
  if (!fp) {
    perror(filename);
    return false;
  }

  int r = fwrite(content.c_str(), content.size(), 1, fp);
  fclose(fp);

  if (r != 1)
    perror(filename);

  return r == 1;
}

///////////////////////////////////////////////////////////////////////
// GBB Utility implementation

namespace vboot_reference {

GoogleBinaryBlockUtil::GoogleBinaryBlockUtil() {
  assert(sizeof(header_) == GBB_HEADER_SIZE);
  initialize();
}

GoogleBinaryBlockUtil::~GoogleBinaryBlockUtil() {
}

void GoogleBinaryBlockUtil::initialize() {
  verbose = true;
  is_valid_gbb = false;
  header_offset_ = 0;
  memset(&header_, 0, sizeof(header_));
  file_content_.clear();
}

bool GoogleBinaryBlockUtil::load_from_file(const char *filename) {
  is_valid_gbb = false;

  file_content_ = read_nonempty_file(filename);
  if (file_content_.empty())
    return false;

  switch (search_header_signatures(file_content_, &header_offset_)) {
    case 0:
      if (verbose)
        fprintf(stderr, " error: cannot find any GBB signature.\n");
      break;

    case 1:
      // fetch a copy of block header to check more detail
      if (!load_gbb_header(file_content_, header_offset_, &header_)) {
        if (verbose)
          fprintf(stderr, " error: invalid GBB in image file.\n");
      } else {
        is_valid_gbb = true;
      }
      break;

    default:
      if (verbose)
        fprintf(stderr, " error: found multiple GBB signatures.\n");
      file_content_.clear();
      break;
  }

  // discard if anything goes wrong
  if (!is_valid_gbb)
    initialize();

  return is_valid_gbb;
}

bool GoogleBinaryBlockUtil::save_to_file(const char *filename) {
  assert(is_valid_gbb && !file_content_.empty());
  return write_nonempty_file(filename, file_content_);
}

int GoogleBinaryBlockUtil::search_header_signatures(const string &image,
                                                    long *poffset) const {
  int found_signatures = 0;
  size_t last_found_pos = 0;

  while ((last_found_pos =
          file_content_.find(GBB_SIGNATURE, last_found_pos, GBB_SIGNATURE_SIZE))
            != file_content_.npos) {
    *poffset = last_found_pos;
    found_signatures++;
    last_found_pos++;  // for next iteration
  }

  return found_signatures;
}

// uility function for load_gbb_header to check property range
static bool check_property_range(uint32_t off, uint32_t sz,
                                 uint32_t hdr_sz, uint32_t max_sz,
                                 const char *prop_name, bool verbose) {
  if (off + sz > max_sz) {
    if (verbose)
      fprintf(stderr, " error: property %s exceed GBB.\n", prop_name);
    return false;
  }

  if (off < hdr_sz) {
    if (verbose)
      fprintf(stderr, " error: property %s overlap GBB header.\n", prop_name);
    return false;
  }

  return true;
}

bool GoogleBinaryBlockUtil::load_gbb_header(const string &image, long offset,
                                         GoogleBinaryBlockHeader *phdr) const {
  assert(phdr);

  // check that GBB header does not extend past end of image
  if (image.size() < (size_t)offset + GBB_HEADER_SIZE) {
    if (verbose)
      fprintf(stderr, " error: incomplete GBB.\n");
    return false;
  }

  string::const_iterator block_ptr = image.begin() + offset;
  size_t block_size = image.size() - offset;

  std::copy(block_ptr, block_ptr + GBB_HEADER_SIZE,
      reinterpret_cast<char*>(phdr));

  const GoogleBinaryBlockHeader &h = *phdr;  // for quick access

  // check version
  if (h.major_version != GBB_MAJOR_VER ||
      h.minor_version != GBB_MINOR_VER) {
    if (verbose)
      fprintf(stderr, " error: invalid GBB version (%d.%d)\n",
          h.major_version, h.minor_version);
    return false;
  }

  if (h.header_size < GBB_HEADER_SIZE) {
    if (verbose)
      fprintf(stderr, " error: incompatible header size (%d < %d)\n",
          h.header_size, GBB_HEADER_SIZE);
    return false;
  }

  // verify location of properties
  if (!check_property_range(h.hwid_offset, h.hwid_size,
                            h.header_size, block_size, "hwid", verbose) ||
      !check_property_range(h.rootkey_offset, h.rootkey_size,
                            h.header_size, block_size, "rootkey", verbose) ||
      !check_property_range(h.bmpfv_offset, h.bmpfv_size,
                            h.header_size, block_size, "bmpfv", verbose)) {
    return false;
  }

  return true;
}

bool GoogleBinaryBlockUtil::find_property(PROPINDEX i,
                                          uint32_t *poffset,
                                          uint32_t *psize) const {
  switch (i) {
    case PROP_HWID:
      *poffset = header_.hwid_offset;
      *psize = header_.hwid_size;
      break;

    case PROP_ROOTKEY:
      *poffset = header_.rootkey_offset;
      *psize = header_.rootkey_size;
      break;

    case PROP_BMPFV:
      *poffset = header_.bmpfv_offset;
      *psize = header_.bmpfv_size;
      break;

    default:
      assert(!"invalid property index.");
      return false;
  }

  return true;
}

bool GoogleBinaryBlockUtil::set_property(PROPINDEX i, const string &value) {
  uint32_t prop_size;
  uint32_t prop_offset;

  assert(is_valid_gbb);

  if (!find_property(i, &prop_offset, &prop_size)) {
    if (verbose)
      fprintf(stderr, " internal error: unknown property (%d).\n",
              static_cast<int>(i));
    return false;
  }

  if (prop_size < value.size()) {
    if (verbose)
      fprintf(stderr, " error: value size (%zu) exceed capacity (%u).\n",
          value.size(), prop_size);
    return false;
  }

  if (i == PROP_HWID && prop_size == value.size()) {
    // special case: this is NUL-terminated so it's better to keep one more \0
    if (verbose)
      fprintf(stderr, "error: NUL-terminated string exceed capacity (%d)\n",
          prop_size);
    return false;
  }

  string::iterator dest = file_content_.begin() + header_offset_ + prop_offset;
  file_content_.replace(dest, dest+prop_size, prop_size, '\0');  // wipe first
  std::copy(value.begin(), value.end(), dest);

  return true;
}

string GoogleBinaryBlockUtil::get_property(PROPINDEX i) const {
  uint32_t prop_size;
  uint32_t prop_offset;

  assert(is_valid_gbb);

  if (!find_property(i, &prop_offset, &prop_size)) {
    if (verbose)
      fprintf(stderr, " internal error: unknown property (%d).\n",
              static_cast<int>(i));
    return "";
  }

  string::const_iterator dest = file_content_.begin() +
                                header_offset_ + prop_offset;
  return string(dest, dest + prop_size);
}

bool GoogleBinaryBlockUtil::set_hwid(const char *hwid) {
  return set_property(PROP_HWID, hwid);
}

bool GoogleBinaryBlockUtil::set_rootkey(const std::string &value) {
  return set_property(PROP_ROOTKEY, value);
}

bool GoogleBinaryBlockUtil::set_bmpfv(const string &value) {
  return set_property(PROP_BMPFV, value);
}

} // namespace vboot_reference

#ifdef WITH_UTIL_MAIN

///////////////////////////////////////////////////////////////////////
// command line utilities

// utility function: provide usage of this utility and exit.
static void usagehelp_exit(const char *prog_name) {
  printf(
    "Utility to manage Google Binary Block (GBB)\n"
    "Usage: %s [-g|-s] [OPTIONS] bios_file [output_file]\n\n"
    "-g, --get            \tGet (read) from bios_file, "
                            "with following options:\n"
    "     --hwid          \tReport hardware id (default).\n"
    " -k, --rootkey=FILE  \tFile name to export Root Key.\n"
    " -b, --bmpfv=FILE    \tFile name to export Bitmap FV.\n"
    "\n"
    "-s, --set            \tSet (write) to bios_file, "
                            "with following options:\n"
    " -i, --hwid=HWID     \tThe new hardware id to be changed.\n"
    " -k, --rootkey=FILE  \tFile name of new Root Key.\n"
    " -b, --bmpfv=FILE    \tFile name of new Bitmap FV\n"
    "\n"
    " SAMPLE:\n"
    "  %s -g bios.bin\n"
    "  %s --set --hwid='New Model' -k key.bin bios.bin newbios.bin\n"
    , prog_name, prog_name, prog_name);
  exit(1);
}

// utility function: export a property from GBB to given file.
// return true on success, otherwise false.
static bool export_property_to_file(const string &filename,
                                    const char *name, const string &value) {
  assert(!filename.empty());
  const char *fn = filename.c_str();

  if (!write_nonempty_file(fn, value)) {
    fprintf(stderr, "error: failed to export %s to file: %s\n", name, fn);
    return false;
  }

  printf(" - exported %s to file: %s\n", name, fn);
  return true;
}

// utility function: import a property to GBB by given file.
// return true on success, otherwise false.
// is succesfully imported into GBB.
static bool import_property_from_file(
    const string &filename, const char *name,
    bool (vboot_reference::GoogleBinaryBlockUtil::*setter)(const string &value),
    vboot_reference::GoogleBinaryBlockUtil *putil) {
  assert(!filename.empty());

  printf(" - import %s from %s: ", name, filename.c_str());
  string v = read_nonempty_file(filename.c_str());
  if (v.empty()) {
    printf("invalid file.\n");
    return false;
  }

  if (!(putil->*setter)(v)) {
    printf("invalid content.\n");
    return false;
  }

  printf("success.\n");
  return true;
}

///////////////////////////////////////////////////////////////////////
// main

int main(int argc, char *argv[]) {
  const char *myname = argv[0];
  int err_stage = 0;    // an indicator for error exits

  struct GBBUtilOptions {
    bool get_mode, set_mode;
    bool use_hwid, use_rootkey, use_bmpfv;
    string hwid, rootkey_fn, bmpfv_fn;
  } myopts;

  myopts.get_mode = myopts.set_mode = false;
  myopts.use_hwid = myopts.use_rootkey = myopts.use_bmpfv = false;

  // snippets for getopt_long
  int option_index, opt;
  static struct option long_options[] = {
    {"get",     0, NULL, 'g' },
    {"set",     0, NULL, 's' },
    {"hwid",    2, NULL, 'i' },
    {"rootkey", 1, NULL, 'k' },
    {"bmpfv",   1, NULL, 'b' },
    { NULL,     0, NULL, 0           },
  };
  int opt_props = 0; // number of assigned properties.

  // parse command line options
  while ((opt = getopt_long(argc, argv, "gsi:k:b:",
                            long_options, &option_index)) >= 0) {
    switch (opt) {
      case 'g':
        myopts.get_mode = true;
        break;

      case 's':
        myopts.set_mode = true;
        break;

      case 'i':
        opt_props++;
        myopts.use_hwid = true;
        if (optarg)
          myopts.hwid = optarg;
        break;

      case 'k':
        opt_props++;
        myopts.use_rootkey = true;
        myopts.rootkey_fn = optarg;
        break;

      case 'b':
        opt_props++;
        myopts.use_bmpfv = true;
        myopts.bmpfv_fn = optarg;
        break;

      default:
      case '?':
        usagehelp_exit(myname);
        break;
    }
  }
  argc -= optind;
  argv += optind;

  // check parameters configuration
  if (!(argc == 1 || (myopts.set_mode && argc == 2)))
    usagehelp_exit(myname);

  // stage: parameter parsing
  err_stage++;
  if (myopts.get_mode == myopts.set_mode) {
    printf("error: please assign either get or set mode.\n");
    return err_stage;
  }

  // stage: load image files
  err_stage++;
  vboot_reference::GoogleBinaryBlockUtil util;
  const char *input_filename = argv[0],
             *output_filename= (argc > 1) ? argv[1] : argv[0];

  if (!util.load_from_file(input_filename)) {
    printf("error: cannot load valid BIOS file: %s\n", input_filename);
    return err_stage;
  }

  // stage: processing by mode
  err_stage++;
  if (myopts.get_mode) {
    // get mode
    if (opt_props < 1)  // enable hwid by default
      myopts.use_hwid = true;

    if (myopts.use_hwid)
      printf("Hardware ID: %s\n", util.get_hwid().c_str());
    if (myopts.use_rootkey)
      export_property_to_file(myopts.rootkey_fn, "rootkey", util.get_rootkey());
    if (myopts.use_bmpfv)
      export_property_to_file(myopts.bmpfv_fn,   "bmpfv",   util.get_bmpfv());
  } else {
    // set mode
    assert(myopts.set_mode);
    if (opt_props < 1) {
      printf("nothing to change. abort.\n");
      return err_stage;
    }

    // HWID does not come from file, so update it direcly here.
    if (myopts.use_hwid) {
      string old_hwid = util.get_hwid();
      if (!util.set_hwid(myopts.hwid.c_str())) {
        printf("error: inproper hardware id: %s\n",
            myopts.hwid.c_str());
        return err_stage;
      }
      printf(" - Hardware id changed: %s -> %s.\n",
          old_hwid.c_str(), util.get_hwid().c_str());
    }

    // import other properties from file
    if ((myopts.use_rootkey &&
         !import_property_from_file(myopts.rootkey_fn, "rootkey",
          &vboot_reference::GoogleBinaryBlockUtil::set_rootkey, &util)) ||
        (myopts.use_bmpfv &&
         !import_property_from_file(myopts.bmpfv_fn, "bmpfv",
          &vboot_reference::GoogleBinaryBlockUtil::set_bmpfv, &util))) {
      printf("error: cannot set new properties. abort.\n");
      return err_stage;
    }

    // stage: write output
    err_stage++;
    if (!util.save_to_file(output_filename)) {
      printf("error: cannot save to file: %s\n", output_filename);
      return err_stage;
    } else {
      printf("successfully saved new image to: %s\n", output_filename);
    }
  }

  return 0;
}

#endif  // WITH_UTIL_MAIN

