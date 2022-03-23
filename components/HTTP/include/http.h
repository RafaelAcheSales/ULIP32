#ifndef __HTTPCLIENT_H__
#define __HTTPCLIENT_H__

typedef void (*http_callback)(const char *url, int status,
                              const char *body, int size);

void http_init(const char *agent);
void http_release(void);

void http_raw_request(const char *hostname, int port, bool secure,
                      const char *auth, const char *path, const char *post_data,
                      const char *headers, int retries, http_callback user_callback);

#endif  /* __HTTPCLIENT_H__ */
