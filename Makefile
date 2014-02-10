ORG  := com.nanoant
NAME := keyremapd
PREFIX := /usr/local

all: $(NAME)

$(NAME): $(NAME).c
	$(CC) $(NAME).c -g -o $(NAME) -framework AppKit

clean:
	rm -rf $(NAME) $(NAME).dSYM

install: $(NAME) $(ORG).$(NAME).plist
	install -d $(PREFIX)/sbin
	install -p $(NAME) $(PREFIX)/sbin
	cp $(ORG).$(NAME).plist /Library/LaunchDaemons

uninstall:
	rm $(PREFIX)/sbin/$(NAME)
	rm -f /Library/LaunchDaemons/$(ORG).$(NAME).plist

start:
	launchctl load -wF /Library/LaunchDaemons/$(ORG).$(NAME).plist
	launchctl start $(ORG).$(NAME)

stop:
	launchctl stop $(ORG).$(NAME)
	launchctl unload -F /Library/LaunchDaemons/$(ORG).$(NAME).plist
