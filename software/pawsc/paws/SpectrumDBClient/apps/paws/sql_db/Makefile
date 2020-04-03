##
## Copyright (c) Microsoft Corporation. All rights reserved.
## Licensed under the MIT License.
##

include ${PRJROOT}/Makefile.common

DB_SCHEMA=paws_schema.json
COMMON_SRC=common_$(TARGET_PLATFORM).json
DEFAULT_SRC=$(COMMON_SRC) gps_$(TARGET_PLATFORM).json devices_$(TARGET_PLATFORM).json

native_SRC = $(COMMON_SRC) gps_$(TARGET_PLATFORM).json devices_$(TARGET_PLATFORM).json
fap_SRC = $(COMMON_SRC) devices_$(TARGET_PLATFORM).json



db=paws_$(TARGET_PLATFORM).db

.PHONY: db
db: $(db)

$(db): $(DB_SCHEMA) $(DEFAULT_SRC)
	rm -f tmp_db
	python ../../../libs/sql_db/create_db.py --schema "$(DB_SCHEMA)" --data_l "$($(TARGET_PLATFORM)_SRC)" --o tmp_db
	cp -f tmp_db $(db)

.PHONY: natives
natives: native1_override.db

native1_override.db : $(DB_SCHEMA) $(DEFAULT_SRC) override_native.json
	rm -f tmp_db
	python ../../../libs/sql_db/create_db.py --schema "$(DB_SCHEMA)" --data_l "$(DEFAULT_SRC) override_native.json" --o tmp_db
	cp -f tmp_db native1_override.db

