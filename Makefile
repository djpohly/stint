prefix = /usr
destdir =

BINARIES = stint

all: $(BINARIES)

stint: LDLIBS=-lX11

clean:
	$(RM) $(BINARIES)

install: $(BINARIES)
	install -d "$(destdir)$(prefix)/bin"
	install -t "$(destdir)$(prefix)/bin" $^
