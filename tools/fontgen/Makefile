UNAME := $(shell uname)
RUBY=ruby
PROJECTNAME=fontgen
LACE=../../thirdparty/lace/bin/lace

ifeq ($(UNAME), CYGWIN_NT-6.1-WOW64)
	TARGET_PLATFORM=win32
	TARGET_BINARY_SUFFIX=.exe
else
	TARGET_PLATFORM=linux
	TARGET_BINARY_SUFFIX=
endif

debug:
	@$(RUBY) $(LACE) -b $(TARGET_PLATFORM)/debug

release:
	@$(RUBY) $(LACE) -b $(TARGET_PLATFORM)/release

master:
	@$(RUBY) $(LACE) -b $(TARGET_PLATFORM)/master

clean:
	@$(RUBY) $(LACE) -c -b $(TARGET_PLATFORM)/debug
	@$(RUBY) $(LACE) -c -b $(TARGET_PLATFORM)/release
	@$(RUBY) $(LACE) -c -b $(TARGET_PLATFORM)/master

test:
	@$(RUBY) $(LACE) -ba

all: debug release master

run: debug
	@./build/$(TARGET_PLATFORM)/debug/$(PROJECTNAME)$(TARGET_BINARY_SUFFIX)

