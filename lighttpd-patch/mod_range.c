#include "base.h"
#include "log.h"
#include "buffer.h"
#include "response.h"
#include "http_chunk.h"
#include "stat_cache.h"

#include "plugin.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <Python.h>

/* plugin config for all request/connections */

typedef struct {
	array *extensions;
} plugin_config;

typedef struct {
	PLUGIN_DATA;

	buffer *query_str;
	array *get_params;

	plugin_config **config_storage;

	plugin_config conf;
} plugin_data;


static void py_assert(void *p)
{
	if (!p) {
		PyErr_Print();
		exit(1);
	}
}

static PyObject *py_mod, *py_jmp;

/* init the plugin data */
INIT_FUNC(mod_range_init) {
	plugin_data *p;

	p = calloc(1, sizeof(*p));

	p->query_str = buffer_init();
	p->get_params = array_init();

	Py_Initialize();
	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.path.append('/usr/lib/xcache')");

	py_mod = PyImport_ImportModule("util");
	py_assert(py_mod);
	py_jmp = PyObject_GetAttrString(py_mod, "jmp");
	py_assert(py_jmp);

	return p;
}

/* detroy the plugin data */
FREE_FUNC(mod_range_free) {
	plugin_data *p = p_d;

	UNUSED(srv);

	if (!p) return HANDLER_GO_ON;

	if (p->config_storage) {
		size_t i;

		for (i = 0; i < srv->config_context->used; i++) {
			plugin_config *s = p->config_storage[i];

			if (!s) continue;

			array_free(s->extensions);

			free(s);
		}
		free(p->config_storage);
	}

	buffer_free(p->query_str);
	array_free(p->get_params);

	free(p);

	return HANDLER_GO_ON;
}

/* handle plugin config and check values */

SETDEFAULTS_FUNC(mod_range_set_defaults) {
	plugin_data *p = p_d;
	size_t i = 0;

	config_values_t cv[] = {
		{ "flv-streaming.extensions",   NULL, T_CONFIG_ARRAY, T_CONFIG_SCOPE_CONNECTION },       /* 0 */
		{ NULL,                         NULL, T_CONFIG_UNSET, T_CONFIG_SCOPE_UNSET }
	};

	if (!p) return HANDLER_ERROR;

	p->config_storage = calloc(1, srv->config_context->used * sizeof(specific_config *));

	for (i = 0; i < srv->config_context->used; i++) {
		plugin_config *s;

		s = calloc(1, sizeof(plugin_config));
		s->extensions     = array_init();

		cv[0].destination = s->extensions;

		p->config_storage[i] = s;

		if (0 != config_insert_values_global(srv, ((data_config *)srv->config_context->data[i])->value, cv)) {
			return HANDLER_ERROR;
		}
	}

	return HANDLER_GO_ON;
}

#define PATCH(x) \
	p->conf.x = s->x;
static int mod_range_patch_connection(server *srv, connection *con, plugin_data *p) {
	size_t i, j;
	plugin_config *s = p->config_storage[0];

	PATCH(extensions);

	/* skip the first, the global context */
	for (i = 1; i < srv->config_context->used; i++) {
		data_config *dc = (data_config *)srv->config_context->data[i];
		s = p->config_storage[i];

		/* condition didn't match */
		if (!config_check_cond(srv, con, dc)) continue;

		/* merge config */
		for (j = 0; j < dc->value->used; j++) {
			data_unset *du = dc->value->data[j];

			if (buffer_is_equal_string(du->key, CONST_STR_LEN("flv-streaming.extensions"))) {
				PATCH(extensions);
			}
		}
	}

	return 0;
}
#undef PATCH

static int split_get_params(array *get_params, buffer *qrystr) {
	size_t is_key = 1;
	size_t i;
	char *key = NULL, *val = NULL;

	key = qrystr->ptr;

	/* we need the \0 */
	for (i = 0; i < qrystr->used; i++) {
		switch(qrystr->ptr[i]) {
		case '=':
			if (is_key) {
				val = qrystr->ptr + i + 1;

				qrystr->ptr[i] = '\0';

				is_key = 0;
			}

			break;
		case '&':
		case '\0': /* fin symbol */
			if (!is_key) {
				data_string *ds;
				/* we need at least a = since the last & */

				/* terminate the value */
				qrystr->ptr[i] = '\0';

				if (NULL == (ds = (data_string *)array_get_unused_element(get_params, TYPE_STRING))) {
					ds = data_string_init();
				}
				buffer_copy_string_len(ds->key, key, strlen(key));
				buffer_copy_string_len(ds->value, val, strlen(val));

				array_insert_unique(get_params, (data_unset *)ds);
			}

			key = qrystr->ptr + i + 1;
			val = NULL;
			is_key = 1;
			break;
		}
	}

	return 0;
}

static int call_jmp(
		connection *con, server *srv, 
		char *path, char *qry, char *ran)
{
	PyObject *r = PyObject_CallFunction(py_jmp, "sss", path, qry, ran);
	//log_error_write(srv, __FILE__, __LINE__, "d", r);
	PyObject *r1 = PyTuple_GetItem(r, 0);
	PyObject *r2 = PyTuple_GetItem(r, 1);
	char *s1 = PyString_AsString(r1);
	log_error_write(srv, __FILE__, __LINE__, "ss", "jmp=", s1);
	if (*s1 == 'm') {
		int ls = PyList_Size(r2);
		int i;
		log_error_write(srv, __FILE__, __LINE__, "sd", "listsize=", ls);
		con->file_finished = 1;
		for (i = 0; i < ls; i++) {
			PyObject *t = PyList_GetItem(r2, i);
			PyObject *t1 = PyTuple_GetItem(t, 0);
			PyObject *t2 = PyTuple_GetItem(t, 1);
			PyObject *t3 = PyTuple_GetItem(t, 2);
			char *fpath = PyString_AsString(t1);
			int start = PyInt_AsLong(t2);
			int len = PyInt_AsLong(t3);
			log_error_write(srv, __FILE__, __LINE__, "sdsdd", 
						"flist", i, fpath, start, len);
			buffer *b = buffer_init_string(fpath);
			http_chunk_append_file(srv, con, b, start, len);
			buffer_free(b);
		}
		return HANDLER_FINISHED;
	} else {
		char *s2 = PyString_AsString(r2);
		con->http_status = 302;
		response_header_insert(srv, con, 
				CONST_STR_LEN("Location"), 
				s2, strlen(s2));
		log_error_write(srv, __FILE__, __LINE__, "ss", "302->", s2);
		return HANDLER_FINISHED;
	}
}

URIHANDLER_FUNC(mod_range_path_handler) {
	plugin_data *p = p_d;
	int s_len;
	size_t k;

	char *s = con->uri.query->ptr;
	log_error_write(srv, __FILE__, __LINE__, "ss", "query=", s);
	log_error_write(srv, __FILE__, __LINE__, "ss", "range=", 
			con->request.http_range);
	if (s && strstr(s, "yjwt")) {
		return call_jmp(con, srv, 
				con->uri.path->ptr, s, con->request.http_range);
		/*
		char *ss = strstr(s, "302");
		if (ss) {
			con->http_status = 302;
			response_header_insert(srv, con, CONST_STR_LEN("Location"), 
					CONST_STR_LEN("http://g.cn"));
			return HANDLER_FINISHED;
		}
		ss = strstr(s, "chunk");
		if (ss) {
			con->file_finished = 1;
			buffer *b = buffer_init_string("/tmp/ha");
			http_chunk_append_file(srv, con, b, 0, 3);
			buffer_free(b);
			b = buffer_init_string("/tmp/hb");
			http_chunk_append_file(srv, con, b, 0, 3);
			buffer_free(b);
			return HANDLER_FINISHED;
		}
		ss = strstr(s, "py");
		if (ss) {
			con->http_status = 302;
			//log_error_write(srv, __FILE__, __LINE__, "d", py_jmp);
			PyObject *r = PyObject_CallFunction(py_jmp, "k", 1);
			//log_error_write(srv, __FILE__, __LINE__, "d", r);
			PyObject *r1 = PyTuple_GetItem(r, 0);
			PyObject *r2 = PyTuple_GetItem(r, 1);
			PyObject *r3 = PyTuple_GetItem(r, 2);
			char *s1 = PyString_AsString(r1);
			char *s2 = PyString_AsString(r2);
			int ls = PyList_Size(r3);
			int i;
			for (i = 0; i < ls; i++) {
				PyObject *t = PyList_GetItem(r3, i);
				PyObject *t1 = PyTuple_GetItem(t, 0);
				PyObject *t2 = PyTuple_GetItem(t, 1);
				PyObject *t3 = PyTuple_GetItem(t, 2);
				PyObject *t4 = PyTuple_GetItem(t, 3);
				int start = PyInt_AsLong(t1);
				int end = PyInt_AsLong(t2);
				char *path = PyString_AsString(t3);
				int hl = PyInt_AsLong(t4);
				log_error_write(srv, __FILE__, __LINE__, "dsddd", 
						i, path, start, end, hl);
			}
			//log_error_write(srv, __FILE__, __LINE__, "ss", s1, s2);
			return HANDLER_FINISHED;
		}
		*/
	}

	if (0) {
		char *ss = strstr(s, "rs=");
		char *es = strstr(s, "rl=");
		//log_error_write(srv, __FILE__, __LINE__, "s", s);
		if (ss && es) {
			http_chunk_append_file(srv, con, con->physical.path, 
					atoi(ss+3), atoi(es+3));
			con->file_finished = 1;
			return HANDLER_FINISHED;
		}
	}

	return HANDLER_GO_ON;

	UNUSED(srv);

	if (con->mode != DIRECT) return HANDLER_GO_ON;

	if (buffer_is_empty(con->physical.path)) return HANDLER_GO_ON;

	mod_range_patch_connection(srv, con, p);

	s_len = con->physical.path->used - 1;

	for (k = 0; k < p->conf.extensions->used; k++) {
		data_string *ds = (data_string *)p->conf.extensions->data[k];
		int ct_len = ds->value->used - 1;

		if (ct_len > s_len) continue;
		if (ds->value->used == 0) continue;

		if (0 == strncmp(con->physical.path->ptr + s_len - ct_len, ds->value->ptr, ct_len)) {
			data_string *get_param;
			stat_cache_entry *sce = NULL;
			buffer *b;
			int start;
			char *err = NULL;
			/* if there is a start=[0-9]+ in the header use it as start,
			 * otherwise send the full file */

			array_reset(p->get_params);
			buffer_copy_string_buffer(p->query_str, con->uri.query);
			split_get_params(p->get_params, p->query_str);

			if (NULL == (get_param = (data_string *)array_get_element(p->get_params, "start"))) {
				return HANDLER_GO_ON;
			}

			/* too short */
			if (get_param->value->used < 2) return HANDLER_GO_ON;

			/* check if it is a number */
			start = strtol(get_param->value->ptr, &err, 10);
			if (*err != '\0') {
				return HANDLER_GO_ON;
			}

			if (start <= 0) return HANDLER_GO_ON;

			/* check if start is > filesize */
			if (HANDLER_GO_ON != stat_cache_get_entry(srv, con, con->physical.path, &sce)) {
				return HANDLER_GO_ON;
			}

			if (start > sce->st.st_size) {
				return HANDLER_GO_ON;
			}

			/* we are safe now, let's build a flv header */
			b = chunkqueue_get_append_buffer(con->write_queue);
			buffer_copy_string_len(b, CONST_STR_LEN("FLV\x1\x1\0\0\0\x9\0\0\0\x9"));

			http_chunk_append_file(srv, con, con->physical.path, start, sce->st.st_size - start);

			response_header_overwrite(srv, con, CONST_STR_LEN("Content-Type"), CONST_STR_LEN("video/x-flv"));

			con->file_finished = 1;

			return HANDLER_FINISHED;
		}
	}
	http_chunk_append_file(srv, con, con->physical.path, 0, 3);
	con->file_finished = 1;
	return HANDLER_FINISHED;
	/* not found */
	return HANDLER_GO_ON;
}

/* this function is called at dlopen() time and inits the callbacks */

int mod_range_plugin_init(plugin *p);
int mod_range_plugin_init(plugin *p) {
	p->version     = LIGHTTPD_VERSION_ID;
	p->name        = buffer_init_string("range");

	p->init        = mod_range_init;
	p->handle_physical = mod_range_path_handler;
	p->set_defaults  = mod_range_set_defaults;
	p->cleanup     = mod_range_free;

	p->data        = NULL;

	return 0;
}
