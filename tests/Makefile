CC ?= cc
CFLAGS ?= -O1 -g

.PHONY: check
check:
	@binary=$$(mktemp /tmp/vendor-reset-test.XXXXXX); \
	trap 'rm -f "$$binary"' EXIT; \
	$(CC) $(CFLAGS) -std=gnu11 -Wall -Wextra -Werror \
	  -Wno-unused-parameter -Wno-unused-variable \
	  -fsanitize=address,undefined \
	  -Iinclude -I../src -I../src/amd/amdgpu \
	  -I../src/amd/amdgpu/include navi48.c device-db.c -o "$$binary" && "$$binary"
