# do not remove intermediate targets
.SECONDARY:

ifndef version
version := 1.XX
endif

name := KungFuSIDUpdater

obj :=
obj += build/main.o
obj += build/base.o
obj += build/screen.o
obj += build/kff_data.o

inc      := .

INCLUDE  := $(addprefix -I,$(inc))
DEFINE   := -DKFF_VER=\"${version}\"

.PHONY: all
all: build/$(name).prg

###############################################################################
# Poor men's dependencies: Let all files depend from all header files
#
headers := $(wildcard *.h)

build/%.s: %.c $(headers) | build
	cc65 -t c64 -T -O --static-locals -g $(INCLUDE) $(DEFINE) -o $@ $<

###############################################################################
build/%.o: build/%.s | build
	ca65 -t c64 -g $(INCLUDE) -o $@ $<

###############################################################################
build/%.o: %.s | build
	ca65 -t c64 -g $(INCLUDE) -o $@ $<

###############################################################################
#
$(libef3usb): force
	@$(MAKE) -C $(dir $@)

.PHONY: force
force:

###############################################################################
build:
	@mkdir -p $@

build/$(name).prg: $(obj) $(libef3usb) | build
	cl65 -t c64 -o $@ $(obj)

.PHONY: clean
clean:
	rm -rf build
	@-$(MAKE) -C $(dir $(libef3usb)) clean
