CFLAGS = -Wall -pipe $(EXTRA_CFLAGS)
LDFLAGS=$(EXTRA_LDFLAGS)

OBJS =admlink_main.o admlink_socket.o admlink_misc.o admlink_sm.o admlink_ctrl_if.o admlink_msghdl.o
SRCOBJS = src/eloop.o src/cJSON.o src/mqtt.o src/mqtt_pal.o
DEFINE+= -D MQTT_USE_BIO -D AG_DBG
CLI_OBJS = admlink-ctrl/admlink_ctrl.o admlink-ctrl/admlink_ctrl_sock.o

INCLUDE = -I./include
#### SSL ####
LIBS =  -lssl -lcrypto
#### cJSON ####
LIBS += -lm
#### PTHREAD ####
#LIBS += -lpthread
#### CURL ####
LIBS += -lcurl
all: clean src admlink admlink_ctrl

admlink:  $(OBJS)
	@echo 'CC $@'
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ $? $(SRCOBJS) $(LIBS); \
	ls -al admlink;
src:  $(SRCOBJS)
	
admlink_ctrl: $(CLI_OBJS)
	@echo 'CC $@'
	@$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(CLI_OBJS); \
	ls -al admlink_ctrl;

%.o: %.c
	@echo 'CC $@'
	$(CC) $(DEFINE) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	cd src && rm -f $(EXECS) *.elf *.gdb *.o *~ *.a 
	cd admlink-ctrl && rm -f $(EXECS) *.elf *.gdb *.o *~ *.a 
	rm -f $(EXECS) *.elf *.gdb *.o *~ *.a admlink
	rm -f $(EXECS) *.elf *.gdb *.o *~ *.a admlink_ctrl

install: 
	mkdir -p $(PREFIX)/sbin; \
	install admlink $(PREFIX)/sbin; 
	install admlink_ctrl $(PREFIX)/sbin; 
	
