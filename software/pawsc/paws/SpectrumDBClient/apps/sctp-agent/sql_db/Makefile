##
# Copyright(c) Microsoft Corporation.All rights reserved.
# Licensed under the MIT License.
##

include ${PRJROOT}/Makefile.common

DB_SCHEMA=sctp_schema.json
COMMON_SRC=common_$(TARGET_PLATFORM).json
DEFAULT_SRC=$(COMMON_SRC) sockets_$(TARGET_PLATFORM).json 

db=sctp_agent_$(TARGET_PLATFORM).db

.PHONY: all 
all: db

.PHONY: db
db: $(db) 

$(db): $(DB_SCHEMA) $(DEFAULT_SRC)
	rm -f tmp_db
	python ../../../libs/sql_db/create_db.py --schema "$(DB_SCHEMA)" --data_l "$(DEFAULT_SRC)" --o tmp_db 
	cp -f tmp_db $(db)

