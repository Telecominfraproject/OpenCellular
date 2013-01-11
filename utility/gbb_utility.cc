// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Utility for manipulating Google Binary Block (GBB)
//

#include "gbb_utility.h"

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
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

// utility function: convert integer to little-endian encoded bytes
// return the byte array in string type
static string int2bytes(const uint32_t value) {
  const char *pvalue = reinterpret_cast<const char*>(&value);
  return string(pvalue, sizeof(value));
}

// utility function: convert little-endian encoded bytes to integer
// return value in uint32_t type
static uint32_t bytes2int(const string &bytes) {
  assert(bytes.size() == sizeof(uint32_t));
  return *reinterpret_cast<const uint32_t*>(bytes.c_str());
}

// utility function: compare a GBB header with given version numbers.
// return 1 for "larger", 0 for "equal" and -1 for "smaller".
static int version_compare(const GoogleBinaryBlockHeader& header,
                           int major, int minor) {
  if (header.major_version != major)
    return header.major_version - major;
  return header.minor_version - minor;
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

bool GoogleBinaryBlockUtil::create_new(
    const std::vector<uint32_t> &create_param) {
  uint32_t *prop = &header_.hwid_offset;  // must be first entry.
  uint32_t allocated_size = sizeof(header_);
  std::vector<uint32_t>::const_iterator i = create_param.begin();

  // max properties = available space in header / size of record (offset+size)
  size_t max_properties =
      (sizeof(header_) - (reinterpret_cast<uint8_t*>(prop) -
                          reinterpret_cast<uint8_t*>(&header_))) /
      (sizeof(uint32_t) * 2);

  if (create_param.size() >= max_properties) {
    if (verbose)
      fprintf(stderr, "error: creation parameters cannot exceed %zu entries.\n",
              max_properties);
    return false;
  }

  initialize();
  memcpy(header_.signature, GBB_SIGNATURE, GBB_SIGNATURE_SIZE);
  header_.major_version = GBB_MAJOR_VER;
  header_.minor_version = GBB_MINOR_VER;
  header_.header_size = GBB_HEADER_SIZE;

  while (i != create_param.end()) {
    *prop++ = allocated_size;  // property offset
    *prop++ = *i;  // property size
    allocated_size += *i;
    i++;
  }

  file_content_.resize(allocated_size);
  std::copy(reinterpret_cast<char*>(&header_),
            reinterpret_cast<char*>(&header_ + 1),
            file_content_.begin());
  is_valid_gbb = true;
  return true;
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

// utility function for load_gbb_header to check property range
static bool check_property_range(uint32_t off, uint32_t sz,
                                 uint32_t hdr_sz, uint32_t max_sz,
                                 const char *prop_name, bool verbose) {
  // for backward compatibility, we allow zero entry here.
  if (off == 0 && sz == 0) {
    if (verbose)
      fprintf(stderr, " warning: property %s is EMPTY.\n", prop_name);
    return true;
  }

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
  if (h.major_version != GBB_MAJOR_VER) {
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

  // verify properties
  for (int i = 0; i < PROP_RANGE; i++) {
    uint32_t off, size;
    const char *name;

    if (!find_property(static_cast<PROPINDEX>(i),
          &off, &size, &name)) {
      assert(!"invalid property.");
      return false;
    }

    if (!check_property_range(off, size,
          h.header_size, block_size, name, verbose))
      return false;
  }

  return true;
}

bool GoogleBinaryBlockUtil::find_property(PROPINDEX i,
                                          uint32_t *poffset,
                                          uint32_t *psize,
                                          const char** pname) const {
  switch (i) {
    case PROP_FLAGS:
      *poffset = (uint8_t*)&header_.flags - (uint8_t*)&header_;
      *psize = sizeof(header_.flags);
      if (pname)
        *pname = "flags";
      break;

    case PROP_HWID:
      *poffset = header_.hwid_offset;
      *psize = header_.hwid_size;
      if (pname)
        *pname = "hardware_id";
      break;

    case PROP_ROOTKEY:
      *poffset = header_.rootkey_offset;
      *psize = header_.rootkey_size;
      if (pname)
        *pname = "root_key";
      break;

    case PROP_BMPFV:
      *poffset = header_.bmpfv_offset;
      *psize = header_.bmpfv_size;
      if (pname)
        *pname = "bmp_fv";
      break;

    case PROP_RCVKEY:
      *poffset = header_.recovery_key_offset;;
      *psize = header_.recovery_key_size;
      if (pname)
        *pname = "recovery_key";
      break;

    default:
      if (verbose) {
        fprintf(stderr, " internal error: unknown property (%d).\n",
            static_cast<int>(i));
      }
      assert(!"invalid property index.");
      return false;
  }

  return true;
}

bool GoogleBinaryBlockUtil::set_property(PROPINDEX i, const string &value) {
  uint32_t prop_size;
  uint32_t prop_offset;
  const char *prop_name;

  assert(is_valid_gbb);

  if (!find_property(i, &prop_offset, &prop_size, &prop_name))
    return false;

  // special processing by version
  if (version_compare(header_, 1, 1) < 0) {
    if (i == PROP_FLAGS) {
      assert(value.size() == prop.size());
      if (int2bytes(0) != value) {
        if (verbose)
          fprintf(stderr,
                  "error: property %s is not supported on GBB version %d.%d\n",
                  prop_name, header_.major_version, header_.minor_version);
        return false;
      }
    }
  }



  if (prop_size < value.size()) {
    if (verbose)
      fprintf(stderr, "error: value size (%zu) exceed property capacity "
              "(%u): %s\n", value.size(), prop_size, prop_name);
    return false;
  }

  // special properties
  switch (i) {
    case PROP_HWID:
      if (value.size() == prop_size) {
        if (verbose)
          fprintf(stderr,
                  "error: NUL-terminated string exceed capacity (%d): %s\n",
                  prop_size, prop_name);
        return false;
      }
      break;

    case PROP_FLAGS:
      assert(value.size() == prop_size);
      header_.flags = bytes2int(value);
      break;

    default:
      break;
  }

  string::iterator dest = file_content_.begin() + header_offset_ + prop_offset;
  file_content_.replace(dest, dest+prop_size, prop_size, '\0');  // wipe first
  std::copy(value.begin(), value.end(), dest);

  return true;
}

string GoogleBinaryBlockUtil::get_property(PROPINDEX i) const {
  uint32_t prop_size;
  uint32_t prop_offset;
  const char *prop_name;

  assert(is_valid_gbb);

  if (!find_property(i, &prop_offset, &prop_size, &prop_name))
    return "";

  // check range again to allow empty value (for compatbility)
  if (prop_offset == 0 && prop_size == 0) {
    if (verbose)
      fprintf(stderr, " warning: empty property (%d): %s.\n",
              static_cast<int>(i), prop_name);
    return "";
  }

  // special processing by version
  if (version_compare(header_, 1, 1) < 0) {
    if (i == PROP_FLAGS)
      return int2bytes(0);
  }

  string::const_iterator dest = file_content_.begin() +
                                header_offset_ + prop_offset;
  return string(dest, dest + prop_size);
}

string GoogleBinaryBlockUtil::get_property_name(PROPINDEX i) const {
  uint32_t unused_off, unused_size;
  const char *prop_name;

  if (!find_property(i, &unused_off, &unused_size, &prop_name)) {
    assert(!"invalid property index.");
    return "";
  }

  return prop_name;
}

uint32_t GoogleBinaryBlockUtil::get_flags() const {
  return bytes2int(get_property(PROP_FLAGS));
}

bool GoogleBinaryBlockUtil::set_flags(const uint32_t flags) {
  return set_property(PROP_FLAGS, int2bytes(flags));
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

bool GoogleBinaryBlockUtil::set_recovery_key(const string &value) {
  return set_property(PROP_RCVKEY, value);
}

}  // namespace vboot_reference

#ifndef FOR_LIBRARY

///////////////////////////////////////////////////////////////////////
// command line utilities

#include <map>

using vboot_reference::GoogleBinaryBlockUtil;

// utility function: provide usage of this utility and exit.
static void usagehelp_exit(const char *prog_name) {
  fprintf(stderr,
    "Utility to manage Google Binary Block (GBB)\n"
    "Usage: %s [-g|-s|-c] [OPTIONS] bios_file [output_file]\n"
    "\n"
    "GET MODE:\n"
    "-g, --get   (default)\tGet (read) from bios_file, "
                            "with following options:\n"
    "     --hwid          \tReport hardware id (default).\n"
    "     --flags         \tReport header flags.\n"
    " -k, --rootkey=FILE  \tFile name to export Root Key.\n"
    " -b, --bmpfv=FILE    \tFile name to export Bitmap FV.\n"
    "     --recoverykey=FILE\tFile name to export Recovery Key.\n"
    "\n"
    "SET MODE:\n"
    "-s, --set            \tSet (write) to bios_file, "
                            "with following options:\n"
    " -o, --output=FILE   \tNew file name for ouptput.\n"
    " -i, --hwid=HWID     \tThe new hardware id to be changed.\n"
    "     --flags=FLAGS   \tThe new (numeric) flags value.\n"
    " -k, --rootkey=FILE  \tFile name of new Root Key.\n"
    " -b, --bmpfv=FILE    \tFile name of new Bitmap FV.\n"
    "     --recoverykey=FILE\tFile name of new Recovery Key.\n"
    "\n"
    "CREATE MODE:\n"
    "-c, --create=prop1_size,prop2_size...\n"
    "                     \tCreate a GBB blob by given size list.\n"
    "SAMPLE:\n"
    "  %s -g bios.bin\n"
    "  %s --set --hwid='New Model' -k key.bin bios.bin newbios.bin\n"
    "  %s -c 0x100,0x1000,0x03DE80,0x1000 gbb.blob\n",
    prog_name, prog_name, prog_name, prog_name);
  exit(1);
}

// utility function: export a property from GBB to given file.
// if filename was empty, export to console (screen).
// return true on success, otherwise false.
static bool export_property(GoogleBinaryBlockUtil::PROPINDEX idx,
                            const string &filename,
                            const GoogleBinaryBlockUtil &util) {
  string prop_name = util.get_property_name(idx),
         value = util.get_property(idx);
  const char *name = prop_name.c_str();

  if (filename.empty()) {
    // write to console
    if (idx == GoogleBinaryBlockUtil::PROP_FLAGS)
      printf("%s: 0x%08x\n", name, bytes2int(value));
    else
      printf("%s: %s\n", name, value.c_str());
  } else {
    const char *fn = filename.c_str();

    if (!write_nonempty_file(fn, value)) {
      fprintf(stderr, "error: failed to export %s to file: %s\n", name, fn);
      return false;
    }
    printf(" - exported %s to file: %s\n", name, fn);
  }

  return true;
}

// utility function: import a property to GBB by given source (file or string).
// return true on success, otherwise false.
// is succesfully imported into GBB.
static bool import_property(
    GoogleBinaryBlockUtil::PROPINDEX idx, const string &source,
    bool source_as_file, GoogleBinaryBlockUtil *putil) {
  assert(!source.empty());
  string prop_name = putil->get_property_name(idx);

  if (source_as_file) {
    printf(" - import %s from %s: ", prop_name.c_str(), source.c_str());
    string v = read_nonempty_file(source.c_str());
    if (v.empty()) {
      printf("invalid file.\n");
      return false;
    }
    if (!putil->set_property(idx, v)) {
      printf("invalid content.\n");
      return false;
    }
    printf("success.\n");
  } else {
    // source as string
    string old_value = putil->get_property(idx);
    bool result = putil->set_property(idx, source);
    if (idx == GoogleBinaryBlockUtil::PROP_FLAGS)
      printf(" - %s changed from 0x%08x to 0x%08x: %s\n",
             prop_name.c_str(), bytes2int(old_value), bytes2int(source),
             result ? "success" : "failed");
    else
      printf(" - %s changed from '%s' to '%s': %s\n",
             prop_name.c_str(), old_value.c_str(), source.c_str(),
             result ? "success" : "failed");
    if (!result)
      return false;
  }

  return true;
}

static bool parse_creation_param(const string &input_string,
                                 std::vector<uint32_t> *output_vector) {
  const char *input = input_string.c_str();
  char *parsed = NULL;
  uint32_t param;

  if (input_string.empty())
    return false;

  do {
    param = (uint32_t)strtol(input, &parsed, 0);
    if (*parsed && *parsed != ',')
      return false;
    output_vector->push_back(param);
    input = parsed + 1;
  } while (*input);

  return true;
}

///////////////////////////////////////////////////////////////////////
// main

int main(int argc, char *argv[]) {
  const char *myname = argv[0];
  int err_stage = 0;    // an indicator for error exits
  GoogleBinaryBlockUtil util;

  // small parameter helper class
  class OptPropertyMap: public
                        std::map<GoogleBinaryBlockUtil::PROPINDEX, string> {
    public:
      bool set_new_value(GoogleBinaryBlockUtil::PROPINDEX id, const string &v) {
        if (find(id) != end())
          return false;

        (*this)[id] = v;
        return true;
      }
  };
  OptPropertyMap opt_props;

  struct GBBUtilOptions {
    bool get_mode, set_mode, create_mode;
    string input_fn, output_fn;
    std::vector<uint32_t> create_param;
  } myopts;
  myopts.get_mode = myopts.set_mode = myopts.create_mode = false;

  // snippets for getopt_long
  int option_index, opt;
  static struct option long_options[] = {
    {"get", 0, NULL, 'g' },
    {"set", 0, NULL, 's' },
    {"create", 1, NULL, 'c' },
    {"output", 1, NULL, 'o' },
    {"hwid", 2, NULL, 'i' },
    {"rootkey", 1, NULL, 'k' },
    {"bmpfv", 1, NULL, 'b' },
    {"recoverykey", 1, NULL, 'R' },
    {"flags", 2, NULL, 'L' },
    { NULL, 0, NULL, 0 },
  };

  // parse command line options
  while ((opt = getopt_long(argc, argv, "gsc:o:i:k:b:",
                            long_options, &option_index)) >= 0) {
    switch (opt) {
      case 'g':
        myopts.get_mode = true;
        break;

      case 's':
        myopts.set_mode = true;
        break;

      case 'c':
        myopts.create_mode = true;
        assert(optarg);
        if (!*optarg || !parse_creation_param(optarg, &myopts.create_param)) {
          fprintf(stderr, "error: invalid creation parameter: %s\n", optarg);
          usagehelp_exit(myname);
        }
        break;

      case 'o':
        myopts.output_fn = optarg;
        break;

      case 'i':
        if (!opt_props.set_new_value(
              GoogleBinaryBlockUtil::PROP_HWID, optarg ? optarg : "")) {
          fprintf(stderr, "error: cannot assign multiple HWID parameters\n");
          usagehelp_exit(myname);
        }
        break;

      case 'k':
        if (!opt_props.set_new_value(
              GoogleBinaryBlockUtil::PROP_ROOTKEY, optarg)) {
          fprintf(stderr, "error: cannot assign multiple rootkey parameters\n");
          usagehelp_exit(myname);
        }
        break;

      case 'b':
        if (!opt_props.set_new_value(
              GoogleBinaryBlockUtil::PROP_BMPFV, optarg)) {
          fprintf(stderr, "error: cannot assign multiple bmpfv parameters\n");
          usagehelp_exit(myname);
        }
        break;

      case 'R':
        if (!opt_props.set_new_value(
              GoogleBinaryBlockUtil::PROP_RCVKEY, optarg)) {
          fprintf(stderr,
                  "error: cannot assign multiple recovery_key parameters\n");
          usagehelp_exit(myname);
        }
        break;

      case 'L':
        {
          uint32_t flags = 0;
          char *endptr = optarg;

          if (optarg) {
            flags = strtoul(optarg, &endptr, 0);
            if (endptr == optarg) {
              fprintf(stderr, "error: invalid --flags value\n");
              usagehelp_exit(myname);
            }
          }

          if (!opt_props.set_new_value(GoogleBinaryBlockUtil::PROP_FLAGS,
                                       optarg ? int2bytes(flags) : "")) {
            fprintf(stderr, "error: cannot assign multiple flags parameters\n");
            usagehelp_exit(myname);
          }
        }
        break;

      default:
      case '?':
        fprintf(stderr, "error: unknown param: %c\n", opt);
        usagehelp_exit(myname);
        break;
    }
  }
  argc -= optind;
  argv += optind;

  // adjust non-dashed parameters
  if (myopts.output_fn.empty() && argc == 2) {
    myopts.output_fn = argv[1];
    argc--;
  }

  // currently, the only parameter is 'input file'.
  if (argc == 1) {
    myopts.input_fn = argv[0];
  } else {
    fprintf(stderr, "error: unexpected parameters (%d)\n", argc);
    usagehelp_exit(myname);
  }

  // stage: complete parameter parsing and checking
  err_stage++;
  if (myopts.create_mode) {
    if (myopts.get_mode || myopts.set_mode) {
      printf("error: please assign only one mode from get/set/create.\n");
      return err_stage;
    }
    if (!opt_props.empty() || myopts.create_param.empty()) {
      printf("error: creation parameter syntax error.\n");
      return err_stage;
    }
    if (myopts.output_fn.empty()) {
      myopts.output_fn = myopts.input_fn;
    }
  } else if (myopts.get_mode == myopts.set_mode) {
    if (myopts.get_mode) {
      printf("error: please assign either get or set mode.\n");
      return err_stage;
    } else {
      // enter 'get' mode by default, if not assigned.
      myopts.get_mode = true;
    }
  }
  if (myopts.get_mode && !myopts.output_fn.empty()) {
    printf("error: get-mode does not create output files.\n");
    return err_stage;
  }

  if (myopts.create_mode) {
    if (!util.create_new(myopts.create_param))
      return err_stage;

    assert(!myopts.output_fn.empty());
    if (!util.save_to_file(myopts.output_fn.c_str())) {
      printf("error: cannot create to file: %s\n", myopts.output_fn.c_str());
      return err_stage;
    } else {
      printf("successfully created new GBB to: %s\n", myopts.output_fn.c_str());
    }
    return 0;
  }

  // stage: load image files
  err_stage++;
  assert(!myopts.input_fn.empty());
  if (!util.load_from_file(myopts.input_fn.c_str())) {
    printf("error: cannot load valid BIOS file: %s\n", myopts.input_fn.c_str());
    return err_stage;
  }

  // stage: processing by mode
  err_stage++;
  if (myopts.get_mode) {
    // get mode
    if (opt_props.empty())  // enable hwid by default
      opt_props.set_new_value(GoogleBinaryBlockUtil::PROP_HWID, "");

    for (OptPropertyMap::const_iterator i = opt_props.begin();
         i != opt_props.end();
         i++) {
      if (i->first == GoogleBinaryBlockUtil::PROP_HWID ||
          i->first == GoogleBinaryBlockUtil::PROP_FLAGS) {
        if (!i->second.empty()) {
          printf("error: cannot assign value for --hwid/flags in --get.\n");
          usagehelp_exit(myname);
        }
      }
      export_property(i->first, i->second, util);
    }

  } else {
    // set mode
    assert(myopts.set_mode);

    if (opt_props.empty()) {
      printf("nothing to change. abort.\n");
      return err_stage;
    }

    for (OptPropertyMap::const_iterator i = opt_props.begin();
         i != opt_props.end();
         i++) {
      bool source_as_file = true;

      // the hwid/flags are assigned in command line parameters
      if (i->first == GoogleBinaryBlockUtil::PROP_HWID ||
          i->first == GoogleBinaryBlockUtil::PROP_FLAGS)
        source_as_file = false;

      if (!import_property(i->first, i->second, source_as_file, &util)) {
        printf("error: cannot set properties. abort.\n");
        return err_stage;
      }
    }

    // stage: write output
    err_stage++;

    // use input filename (overwrite) by default
    if (myopts.output_fn.empty())
      myopts.output_fn = myopts.input_fn;

    assert(!myopts.output_fn.empty());
    if (!util.save_to_file(myopts.output_fn.c_str())) {
      printf("error: cannot save to file: %s\n", myopts.output_fn.c_str());
      return err_stage;
    } else {
      printf("successfully saved new image to: %s\n", myopts.output_fn.c_str());
    }
  }

  return 0;
}

#endif  // FOR_LIBRARY
