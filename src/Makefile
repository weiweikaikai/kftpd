CC=gcc
CFLAGS= -g #-Wall
BIN=kftp
LIB=-lcrypt
OBJS=memory_pool.o  kftpd_log.o string_operate.o config.o parse_config.o  session.o my_hash.o net_connect.o reply.o ipc.o parent_ipc.o parse_ftpproto.o kftpd.o
$(BIN):$(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIB)
%.o:%.c
	$(CC) -c $< -o $@  $(CFLAGS)
clean:
	rm -f *.o $(BIN)
move:
	mv $(BIN) ../bin







