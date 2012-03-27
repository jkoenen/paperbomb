UNAME := $(shell uname)
RUBY=ruby
LACE=../thirdparty/lace/bin/lace

ifeq ($(UNAME), CYGWIN_NT-6.1-WOW64)
	TARGET_PLATFORM=win32
	TARGET_BINARY_SUFFIX=.exe
else
	UPX=../thirdparty/upx/linux/amd64/upx-3.08-amd64_linux/upx -q
	TARGET_PLATFORM=linux
	TARGET_BINARY_SUFFIX=
endif

debug:
	$(RUBY) $(LACE) -b $(TARGET_PLATFORM)/debug

release:
	$(RUBY) $(LACE) -b $(TARGET_PLATFORM)/release

master:
	$(RUBY) $(LACE) -b $(TARGET_PLATFORM)/master
	$(UPX) ./build/$(TARGET_PLATFORM)/master/paperbomb$(TARGET_BINARY_SUFFIX)

clean:
	$(RUBY) $(LACE) -c -b $(TARGET_PLATFORM)/debug
	$(RUBY) $(LACE) -c -b $(TARGET_PLATFORM)/release
	$(RUBY) $(LACE) -c -b $(TARGET_PLATFORM)/master

test:
	$(RUBY) $(LACE) -ba

run: debug
	./build/$(TARGET_PLATFORM)/debug/paperbomb$(TARGET_BINARY_SUFFIX)

