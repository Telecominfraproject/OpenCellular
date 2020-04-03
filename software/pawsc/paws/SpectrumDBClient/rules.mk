#
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
#

ifdef APP

$(APP): $(C_OBJ) libs
	gcc $(CFLAGS) $(INC_DIRS) $(C_OBJ) -o $@ $(C_LIBS)

else

$(LIB): $(C_OBJ)
	ar rs $@ $(C_OBJ)

endif

.PHONY: libs
libs:
	make -C $(PRJROOT)/libs

.PHONY: clean
clean:
	rm -rf obj $(APP) $(LIB)

-include $(DEPS)

obj/$(TARGET_PLATFORM)/%.o: %.c
	@mkdir -p $(dir $@)
	gcc $(CFLAGS) -c $(INC_DIRS) $< -o $@
