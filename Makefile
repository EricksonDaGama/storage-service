
#  Grupo 65
#
#  Vitória Zhu         Nº 56291
#  Su Lishun           Nº 56375
#  Erickson Cacondo    Nº 53653


BIN_DIR = binary
INC_DIR = include
OBJ_DIR = object
SRC_DIR = source
LIB_DIR = lib
PROTOC_DIR = /usr/local

TARGETS = clean sdmessage.pb-c.h tree-server tree-client

OBJ_TREE_SERVER = tree.o tree_skel.o network_server.o message.o data.o entry.o sdmessage.pb-c.o tree_server.o client_stub.o network_client.o
OBJ_TREE_CLIENT = client-lib.o tree_client.o
OBJ_CLIENT_LIB = client_stub.o network_client.o data.o entry.o message.o sdmessage.pb-c.o

data.o = data.h
entry.o = data.h entry.h
tree.o = data.h tree-private.h tree.h
serialization.o = serialization.h

tree_client.o = client_stub.h client_stub-private.h tree_client-private.h
tree_server.o = network_server.h tree_server-private.h tree_skel-private.h
client_stub.o = client_stub.h client_stub-private.h network_client.h sdmessage.pb-c.h
tree_skel.o = tree.h entry.h tree_skel.h tree_skel-private.h client_stub.h client_stub-private.h
network_client.o = inet.h client_stub-private.h message-private.h network_client.h
network_server.o = inet.h tree_skel.h message-private.h network_server.h
message.o = inet.h message-private.h
sdmessage.pb-c.o = sdmessage.pb-c.h

CC = gcc
CFLAGS = -Wall -I $(INC_DIR) -g -pthread
LDFLAGS = -lprotobuf-c -D THREADED -lzookeeper_mt
LINKER = ld -r

all: clean $(TARGETS)

%.pb-c.h: %.proto
	protoc --c_out=${INC_DIR}/ $< && mv ${INC_DIR}/$*.pb-c.c ${SRC_DIR}/

tree-server: $(OBJ_TREE_SERVER)
	$(CC) $(addprefix $(OBJ_DIR)/, $(OBJ_TREE_SERVER)) -I${PROTOC_DIR}/include -L${PROTOC_DIR}/lib ${LDFLAGS} -o $(BIN_DIR)/$@

tree-client: $(OBJ_TREE_CLIENT)
	$(CC) ${LIB_DIR}/client-lib.o ${OBJ_DIR}/tree_client.o -I${PROTOC_DIR}/include -L${PROTOC_DIR}/lib ${LDFLAGS} -o $(BIN_DIR)/$@

client-lib.o: $(OBJ_CLIENT_LIB)
	$(LINKER) $(addprefix $(OBJ_DIR)/, $(OBJ_CLIENT_LIB)) -o $(LIB_DIR)/$@

%.o: $(SRC_DIR)/%.c $($@)
	$(CC) $(CFLAGS) -c $< -o $(OBJ_DIR)/$@

sdmessage.pb-c.o: $(SRC_DIR)/sdmessage.pb-c.c $($@)
	$(CC) $(CFLAGS) -c $< -o $(OBJ_DIR)/$@

clean:
	rm -f $(OBJ_DIR)/*.o $(BIN_DIR)/* ${LIB_DIR}/*.o ${SRC_DIR}/*.pb-c.c ${INC_DIR}/*.pb-c.h