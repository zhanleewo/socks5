#include <common.h>
#include <stdio.h>
#include <server.h>

int main(int argc, char **argv) {
    
    char *host = "0.0.0.0";
    uint16_t *port = 1080;
    int backlog = 10;

    struct sserver_config *config = sserver_config_new(host, port, backlog);
    struct sserver *server = sserver_new(config);
    
    server->start();

    server->stop();
        
	return 0;
}
