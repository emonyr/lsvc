#ifndef __PROTO_TOOL_H__
#define __PROTO_TOOL_H__


typedef enum {
	TYPE_JSON,
	// TYPE_PROTOBUF,
	// TYPE_XML,
	// TYPE_INI,
}proto_type_t;


typedef struct {
	proto_type_t type;
	void *payload;
}__attribute__ ((packed))proto_cache_t;


extern proto_cache_t *proto_init(void *old_cache, proto_type_t type, void *context);
extern int proto_destroy(proto_cache_t *c);
extern void *proto_str_get(proto_cache_t *c, const char *path);
extern int proto_str_check(proto_cache_t *c, const char *path, 
											const char *expect, int strict);

/* json specific api */
extern proto_cache_t *proto_json_init(void *old_cache, char *context);
extern int proto_json_destroy(proto_cache_t *c);
extern int proto_json_cache_dump(proto_cache_t *c, char *bucket);
extern void *proto_json_str_get(proto_cache_t *c, const char *path);
extern int proto_json_str_add(proto_cache_t *c, const char *path, 
									const char *name, const char *string);


#endif	/* __PROTO_TOOL_H__ */