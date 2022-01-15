#ifndef ADMLINK_SOCKET_H
#define ADMLINK_SOCKET_H
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
void open_nb_socket(BIO** bio, SSL_CTX** ssl_ctx, const char* addr, const char* port,
							const char* ca_file, const char* ca_path,
							const char* clt_key, const char* clt_cert);
#endif