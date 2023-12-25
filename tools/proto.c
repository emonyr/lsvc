#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cJSON.h"
#include "mem.h"
#include "proto.h"

#define MAX_NODE_NAME_LEN 128


proto_cache_t *proto_init(void *old_cache, proto_type_t type, void *context)
{
	switch(type) {
		case TYPE_JSON:
			return proto_json_init(old_cache, context);

		// case TYPE_PROTOBUF:
		// 	return proto_protobuf_init(old_cache, context);

		// case TYPE_XML:
		// 	return proto_xml_init(old_cache, context);

		// case TYPE_INI:
		// 	return proto_ini_init(old_cache, context);

	}

	return NULL;
}

int proto_destroy(proto_cache_t *c)
{
	if (!c) {
		fprintf(stderr, "Invalid param\n");
		return -1;
	}

	switch(c->type) {
		case TYPE_JSON:
			return proto_json_destroy(c);

		// case TYPE_PROTOBUF:
		// 	return proto_protobuf_destroy(c);

		// case TYPE_XML:
		// 	return proto_xml_destroy(c);

		// case TYPE_INI:
		// 	return proto_ini_destroy(c);

	}

	return -1;
}

void *proto_str_get(proto_cache_t *c, const char *path)
{
	if (!c || !path) {
		fprintf(stderr, "Invalid param\n");
		return NULL;
	}

	switch(c->type) {
		case TYPE_JSON:
			return proto_json_str_get(c, path);

		// case TYPE_PROTOBUF:
		// 	return proto_protobuf_str_get(c, path);

		// case TYPE_XML:
		// 	return proto_xml_str_get(c, path);

		// case TYPE_INI:
		// 	return proto_ini_str_get(c, path);

	}

	return NULL;
}

double proto_num_get(proto_cache_t *c, const char *path)
{
	if (!c || !path) {
		fprintf(stderr, "Invalid param\n");
		return -1;
	}

	switch(c->type) {
		case TYPE_JSON:
			return proto_json_num_get(c, path);

		// case TYPE_PROTOBUF:
		// 	return proto_protobuf_str_get(c, path);

		// case TYPE_XML:
		// 	return proto_xml_str_get(c, path);

		// case TYPE_INI:
		// 	return proto_ini_str_get(c, path);

	}

	return -1;
}

int proto_str_check(proto_cache_t *c, const char *path,
											const char *expect, int strict)
{
	int match;
	char *text;

	if (!c || !path || !expect) {
		fprintf(stderr, "Invalid param\n");
		return -1;
	}

	text = proto_str_get(c, path);
	if (!text) {
		fprintf(stderr, "Invalid string path: %s\n", path);
		return -1;
	}

	if (strict) {
		match = (text && (strcmp(text, expect) == 0));
	}else{
		match = (text && strstr(text, expect));
	}

	if (!match) {
		fprintf(stderr, "%s(expect) != %s(payload)\n", expect, text ? text : "NULL");
		return -1;
	}

	return 0;
}

void *proto_next_node(const char *path, char *node_name)
{
	int i=0;
	char *node = NULL;
	char *name = NULL;

	if (!path || !node_name) {
		fprintf(stderr, "Invalid path \n");
		return NULL;
	}

	node = strchr(path,'/');
	if (!node || *(node+1) == '\0') {
		return NULL;
	}else{
		node++; //next to '/'
	}

	name = node;
	for(i=0;i<MAX_NODE_NAME_LEN;i++) {
		if (name[i] == '/' || name[i] == '\0')
			break;

		node_name[i] = name[i];
	}
	node_name[i] = '\0';

	return node;
}


/*
 *	json specific api
 */
proto_cache_t *proto_json_init(void *old_cache, char *context)
{
	proto_cache_t *c = old_cache;
	void *payload = NULL;

	if (!context) {
		fprintf(stderr, "Invalid param\n");
		goto out;
	}

	payload = cJSON_Parse(context);
	if (!payload) {
		fprintf(stderr, "Invalid json format\n");
		goto out;
	}

	if (!c) {
		c = mem_alloc(sizeof(proto_cache_t));
		if (!c) {
			fprintf(stderr, "Failed to alloc memory\n");
			cJSON_Delete(payload);
			goto out;
		}
	}else{
		if (c->payload) {
			cJSON_Delete(c->payload);
			c->payload = NULL;
		}
	}

	c->type = TYPE_JSON;
	c->payload = payload;

out:
	return c;
}

int proto_json_destroy(proto_cache_t *c)
{
	if (c && c->payload) {
		cJSON_Delete(c->payload);
		free(c);
	}

	return 0;
}

int proto_json_cache_dump(proto_cache_t *c, char *bucket)
{
	int nbyte=0;
	char *payload = NULL;

	if (!c || !bucket) {
		fprintf(stderr, "Invalid param\n");
		return -1;
	}

	payload = cJSON_PrintUnformatted(c->payload);
	if (!payload) {
		fprintf(stderr, "Invalid payload\n");
		return -1;
	}

	nbyte = sprintf(bucket, "%s", payload);
	bucket[nbyte] = '\0';
	nbyte++;	// including '\0'

	free(payload);

	return nbyte;
}

void *proto_json_search(proto_cache_t *c, const char *path)
{
	char node_name[MAX_NODE_NAME_LEN];
	const char *p_path = NULL;
	cJSON *n=NULL,*m=NULL,*tmp=NULL;
	int index;

	if (!c || !c->payload || !path) {
		fprintf(stderr, "Invalid param\n");
		return NULL;
	}

	m = c->payload;
	p_path = path;

	while((p_path = proto_next_node(p_path,node_name))) {
		//fprintf(stderr, "node_name %s\n", node_name);
		if (cJSON_IsArray(m)) {
			index = -1;
			sscanf(node_name, "[%d]", &index);
			// fprintf(stderr, "index %d\n", index);
			n = (index == -1 ? NULL : cJSON_GetArrayItem(m, index));
		}else{
			n = cJSON_GetObjectItem(m,node_name);
		}

		if (!n) {
			fprintf(stderr, "Node not found: %s\n", node_name);
			return NULL;
		}

		m = n;
	}

	return n;
}

void *proto_json_str_get(proto_cache_t *c, const char *path)
{
	cJSON *n=NULL;

	if (!c || !path) {
		fprintf(stderr, "Invalid param\n");
		return NULL;
	}

	n = proto_json_search(c, path);
	if (!n)
		return NULL;

	return n->valuestring ? n->valuestring : n->string;
}

double proto_json_num_get(proto_cache_t *c, const char *path)
{
	cJSON *n=NULL;

	if (!c || !path) {
		fprintf(stderr, "Invalid param\n");
		return -1;
	}

	n = proto_json_search(c, path);
	if (!n)
		return -1;

	return n->type == cJSON_Number ? (n->valueint?n->valueint:n->valuedouble) : -1;
}

int proto_json_str_add(proto_cache_t *c, const char *path,
									const char *name, const char *string)
{
	cJSON *n=NULL,*m=NULL,*tmp=NULL,*new=NULL;

	if (!c->payload) {
		fprintf(stderr, "Invalid payload\n");
		return -1;
	}

	n = proto_json_search(c, path);
	if (!n)
		n = c->payload;

	m = cJSON_GetObjectItem(n,name);
	if (!m) {
		cJSON_AddStringToObject(n, name, string);
	}else if (cJSON_IsArray(m)) {
		new = cJSON_CreateObject();
		cJSON_AddStringToObject(new, name, string);
		cJSON_AddItemToArray(m,new);
	}else{
		/* switch string object to array object */
		tmp = cJSON_DetachItemFromObjectCaseSensitive(n, name);

		m = cJSON_AddArrayToObject(n, name);

		new = cJSON_CreateObject();
		cJSON_AddStringToObject(new, tmp->string, tmp->valuestring);
		cJSON_AddItemToArray(m,new);

		cJSON_Delete(tmp);

		new = cJSON_CreateObject();
		cJSON_AddStringToObject(new, name, string);
		cJSON_AddItemToArray(m,new);
	}

	return 0;
}

