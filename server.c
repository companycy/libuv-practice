#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

 // gcc  -o server server.c -I/home/bjcheny/libuv-1.x/include/  /home/bjcheny/libuv-1.x/.libs/libuv.a -pthread -lrt

int main1() {
    uv_loop_t *loop = uv_loop_new();

    printf("Now quitting.\n");
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}

typedef struct {
  uv_write_t req;
  uv_buf_t buf;
} write_req_t;


uv_loop_t *loop = NULL;

static void on_close(uv_handle_t* peer) {
  free(peer);
}


void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  *buf = uv_buf_init((char*) malloc(suggested_size), suggested_size);
}

static void after_write(uv_write_t* req, int status) {
  write_req_t* wr = (write_req_t*) req;
  free(wr->buf.base);
  free(wr);
  uv_close((uv_handle_t*)req->handle, on_close);
}

void echo_read(uv_stream_t *server, ssize_t nread, const uv_buf_t *buf) {
  if (nread == -1) {
    fprintf(stderr, "error echo_read");
    return;
  }
 
  printf("result: %s\n", buf->base);

  // uv_buf_t buf1 = uv_buf_init("PING", 4);
  // const int w = uv_try_write((uv_stream_t*)&server, &buf1, 1);
  // printf("write: %d\n", w);
  write_req_t *wr = (write_req_t*) malloc(sizeof (write_req_t));
  wr->buf = uv_buf_init(buf->base, nread); // ("PING", 4);
  if (uv_write(&wr->req, server, &wr->buf, 1, after_write)) {
    fprintf(stderr, "");
  }
  // uv_write(&request, stream, buffer, 2, on_write);

  // const int r = uv_read_start(handle, echo_alloc, after_read_2);
  // printf("read %d\n", r);
}

void on_new_connection(uv_stream_t *server, int status) {
  if (status < 0) {
    fprintf(stderr, "New connection error %s\n", uv_strerror(status));
    return;
  }

  uv_tcp_t *client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
  uv_tcp_init(loop, client);
  if (uv_accept(server, (uv_stream_t*) client) == 0) {
    fprintf(stdout, "accept new connection\n");
    uv_read_start((uv_stream_t*) client, alloc_buffer, echo_read);
  } else {
    uv_close((uv_handle_t*) client, NULL);
  }
}

int main() {
  loop = uv_default_loop();
  uv_tcp_t server;
  uv_tcp_init(loop, &server);
  const int DEFAULT_PORT = 4320;
  struct sockaddr_in addr;
  uv_ip4_addr("0.0.0.0", DEFAULT_PORT, &addr);

  uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
  const int MAXCONN = 1024;
  int r = uv_listen((uv_stream_t*)&server, MAXCONN, on_new_connection);
  if (r) {
    fprintf(stderr, "Listen error %s\n", uv_strerror(r));
    return -1;
  }
  uv_run(loop, UV_RUN_DEFAULT);
  uv_loop_close(loop);
  return 0;
}
