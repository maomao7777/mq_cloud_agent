#include <admlink_socket.h>
#include <admlink.h>
/*****************************************************************************/
/*
    A template for opening a non-blocking OpenSSL connection.
*/
static unsigned char *next_protos_parse(unsigned short *outlen,
                                        const char *in)
{
    size_t len;
    unsigned char *out;
    size_t i, start = 0;

    len = strlen(in);
    if (len >= 65535)
        return NULL;

    out = OPENSSL_malloc(strlen(in) + 1);
    if (!out)
        return NULL;

    for (i = 0; i <= len; ++i) {
        if (i == len || in[i] == ',') {
            if (i - start > 255) {
                OPENSSL_free(out);
                return NULL;
            }
            out[start] = (unsigned char)(i - start);
            start = i + 1;
        } else
            out[i + 1] = in[i];
    }

    *outlen = (unsigned char)(len + 1);
    return out;
}
/*****************************************************************************/
void open_nb_socket(BIO** bio, SSL_CTX** ssl_ctx, const char* addr, const char* port,
					const char* ca_file, const char* ca_path,
				   const char* clt_key, const char* clt_cert) {
    SSL_library_init();
    SSL* ssl;
    *ssl_ctx = SSL_CTX_new(SSLv23_client_method());
	long int sslver_ret;
# if 0
	const char* alpnclt="x-amzn-mqtt-ca";//this is only for 443 port aws broker
	unsigned short alpn_len;
	unsigned char *alpn = next_protos_parse(&alpn_len, alpnclt);
	if (alpn == NULL) 
	{
		printf("Error parsing -alpnclt argument\n");
		exit_link(1);
	}
	SSL_CTX_set_alpn_protos(*ssl_ctx, alpn, alpn_len);
	OPENSSL_free(alpn);
#endif
    /* load certificate */
	if(clt_cert!=NULL
		&&!SSL_CTX_use_certificate_file(*ssl_ctx, clt_cert, SSL_FILETYPE_PEM))
	{
		printf("error: failed to load cert\n");
		exit_link(1);
	}  
	if(clt_key!=NULL
		&&!SSL_CTX_use_PrivateKey_file(*ssl_ctx, clt_key, SSL_FILETYPE_PEM))
	{
		printf("error: failed to load privkey\n");
		exit_link(1);
	} 

    if (ca_file!=NULL
		&&!SSL_CTX_load_verify_locations(*ssl_ctx, ca_file, ca_path)) {
        printf("error: failed to load certificate\n");
		exit_link(1);
    }

    /* open BIO socket */
    *bio = BIO_new_ssl_connect(*ssl_ctx);
    BIO_get_ssl(*bio, &ssl);
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
    BIO_set_conn_hostname(*bio, addr);
    BIO_set_nbio(*bio, 1);
    BIO_set_conn_port(*bio, port);

    /* wait for connect with 10 second timeout */
    int start_time = time(NULL);
    int do_connect_rv = BIO_do_connect(*bio);
    while(do_connect_rv <= 0 && BIO_should_retry(*bio) && (int)time(NULL) - start_time < 10) {
        do_connect_rv = BIO_do_connect(*bio);
    }
    if (do_connect_rv <= 0) {
        printf("bio_do_conect failed: %s\n", ERR_reason_error_string(ERR_get_error()));
        BIO_free_all(*bio);
        SSL_CTX_free(*ssl_ctx);
        *bio = NULL;
        *ssl_ctx=NULL;
        return;
    }

    /* verify certificate */
	sslver_ret=SSL_get_verify_result(ssl);
    if (sslver_ret!= X509_V_OK) {
        /* Handle the failed verification */
        printf("error: x509 certificate verification failed with %ld\n",sslver_ret);
        exit_link(1);
    }
}
/*****************************************************************************/
