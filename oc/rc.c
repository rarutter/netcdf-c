/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ocinternal.h"
#include "ocdebug.h"
#include "ocdata.h"
#include "occontent.h"
#include "oclog.h"

#include "rc.h"

/* These globals are where information from the .dodsrc file is stored. See the
 * functions in curlfunctions.c
 */
struct OCproxy *pstructProxy = NULL;

/* The Username and password are in the URL if the URL is of the form:
 * http://<name>:<passwd>@<host>/....
 */
int
credentials_in_url(const char *url)
{
	char *pos = strstr(url, "http://");
	if (!pos)
		return FALSE;
	pos += 7;
	if (strchr(pos, '@') && strchr(pos, ':'))
		return TRUE;

	return FALSE;
}

int
extract_credentials(const char *url, char **name, char **pw, char **result_url)
{
	char *pos;
	char *end;
	char *middle;
	int up_len = 0;
	int mid_len = 0;
	int midpas_len = 0;
	int url_len = 0;

	if (strchr(url, '@')) {
		pos = strstr(url, "http://");
		if (pos)
			pos += 7;
		middle = strchr(pos, ':');
		mid_len = middle - pos;
		*name = malloc(sizeof(char) * (mid_len + 1));
		strncpy(*name, pos, mid_len);
		(*name)[mid_len] = '\0';

		if (middle)
			middle += 1;

		end = strchr(middle, '@');
		midpas_len = end - middle;
		*pw = malloc(sizeof(char) * (midpas_len + 1));
		strncpy(*pw, middle, midpas_len);
		(*pw)[midpas_len] = '\0';

		up_len = end - pos;
		url_len = strlen(url) - up_len;

		*result_url = malloc(sizeof(char) * (url_len + 1));
		if (!result_url)
			return OC_ENOMEM;

		strncpy(*result_url, url, pos - url);
		strncpy(*result_url + (pos - url), end + 1, url_len - (pos - url));

#if 0
		fprintf(stderr, "URL without username and password: %s:%d\n", sURL, url_len );
		fprintf(stderr, "URL username and password: %s:%d\n", sUP, up_len);
		fprintf(stderr, "URL username: %s:%d\n", sUser, mid_len);
		fprintf(stderr, "URL password: %s:%d\n", sPassword, midpas_len);
#endif
		(*result_url)[url_len] = '\0';

		return OC_NOERR;
	}
	else {
		return OC_EIO;
	}
}

/*Allows for a .dodsrc file to be read in and parsed in order to get authentication information*/
int
read_dodsrc(char *in_file_name, OCstate* state)
{
        char *p;
	char more[1024];
	char *v;
	FILE *in_file;
        char unsupported[4096];

	in_file = fopen(in_file_name, "r"); /* Open the file to read it */
	if (in_file == NULL) {
		oc_log(LOGERR, "Could not open the .dodsrc file");
		return OC_EPERM;
	}

        unsupported[0] = '\0';
	while (fgets(more, 1024, in_file) != NULL) {
		if (more[0] != '#') {
			p = strchr(more, '=');
			v = p + 1;
			*p = '\0';

			if (strcmp(more, "USE_CACHE") == 0) {
				/*strcat(unsupported,",USE_CACHE");*/
			} else if (strcmp(more, "MAX_CACHE_SIZE") == 0) {
				/*strcat(unsupported,",USE_CACHE");*/
			} else if (strcmp(more, "MAX_CACHED_OBJ") == 0) {
				/*strcat(unsupported,",MAX_CACHED_OBJ");*/
			} else if (strcmp(more, "IGNORE_EXPIRES") == 0) {
				/*strcat(unsupported,",IGNORE_EXPIRES");*/
			} else if (strcmp(more, "CACHE_ROOT") == 0) {
				/*strcat(unsupported,",CACHE_ROOT");*/
			} else if (strcmp(more, "DEFAULT_EXPIRES") == 0) {
				/*strcat(unsupported,",DEFAULT_EXPIRES");*/
			} else if (strcmp(more, "ALWAYS_VALIDATE") == 0) {
				/*strcat(unsupported,",ALWAYS_VALIDATE");*/
			} else if (strcmp(more, "DEFLATE") == 0) {
				/* int v_len = strlen(v); unused */
				if(atoi(v)) state->curlflags.compress = 1;
				if (ocdebug > 1)
					oc_log(LOGNOTE,"Compression: %d", state->curlflags.compress);
			} else if (strcmp(more, "VALIDATE_SSL") == 0) {
				if(atoi(v)) state->curlflags.verify = 1;
				if (ocdebug > 1)
					oc_log(LOGNOTE,"SSL Verification: %d", state->curlflags.verify);
			} else if (strcmp(more, "PROXY_SERVER") == 0) {
				char *host_pos = NULL;
				char *port_pos = NULL;
				/* int v_len = strlen(v); unused */

			        if(strlen(v) == 0) continue; /* nothing there*/
				if (credentials_in_url(v)) {
					char *result_url = NULL;
					extract_credentials(v, &state->proxy.username, &state->proxy.password, &result_url);
					v = result_url;
				}
				/* allocating a bit more than likely needed ... */
				host_pos = strstr(v, "http://");
				if (host_pos)
					host_pos += 7;
						
				port_pos = strchr(host_pos, ':');
				if (port_pos) {
					int host_len;
					char *port_sep = port_pos;
					port_pos++;
					*port_sep = '\0';
					host_len = strlen(host_pos);
					state->proxy.host = malloc(sizeof(char) * host_len + 1);
					if (!state->proxy.host)
						return OC_ENOMEM;

					strncpy(state->proxy.host, host_pos, host_len);
					state->proxy.host[host_len + 1] = '\0';

					state->proxy.port = atoi(port_pos);
				}
				else {
					int host_len = strlen(host_pos);
					state->proxy.host = malloc(sizeof(char) * host_len + 1);
					if (!state->proxy.host)
						return OC_ENOMEM;

					strncpy(state->proxy.host, host_pos, host_len);
					state->proxy.host[host_len + 1] = '\0';

					state->proxy.port = 80;
				}
#if 0
				state->proxy.host[v_len] = '\0';

				state->proxy.port = atoi(v);

				s_len = strlen(v);
				state->proxy.user = malloc(sizeof(char) * s_len + 1);
				if (!state->proxy.user)
					return OC_ENOMEM;
				strncpy(state->proxy.user, v, s_len);
				state->proxy.user[s_len] = '\0';

				p_len = strlen(v);
				state->proxy.password = malloc(sizeof(char) * p_len + 1);
				if (!state->proxy.password)
					return OC_ENOMEM;
				strncpy(state->proxy.password, v, p_len);
				state->proxy.password[p_len] = '\0';
#endif
				if (ocdebug > 1) {
					oc_log(LOGNOTE,"host name: %s", state->proxy.host);
					oc_log(LOGNOTE,"user name: %s", state->proxy.username);
					oc_log(LOGNOTE,"password name: %s", state->proxy.password);
					oc_log(LOGNOTE,"port number: %d", state->proxy.port);
				}

			} else if (strcmp(more, "NO_PROXY_FOR") == 0) {
				/*strcat(unsupported,",NO_PROXY_FOR");*/
			} else if (strcmp(more, "AIS_DATABASE") == 0) {
				/*strcat(unsupported,",AIS_DATABASE");*/
			} else if (strcmp(more, "COOKIE_JAR") == 0) {
				int v_len = strlen(v);
				state->curlflags.cookies = malloc(sizeof(char) * v_len + 1);
				if (!state->curlflags.cookies)
					return OC_ENOMEM;
				strncpy(state->curlflags.cookies, v, v_len);
				state->curlflags.cookies[v_len] = '\0';
				if (ocdebug > 1)
					oc_log(LOGNOTE,"Cookie jar name: %s", state->curlflags.cookies);
			}

		}
	}
	fclose(in_file);

#ifdef IGNORE
	if(unsupported[0] != '\0') {
	    unsupported[0] = ' '; /* Elide leading comma */
	    oc_log(LOGNOTE,"Not currently supported in .dodsrc: %s",unsupported);
	}
#endif

	return OC_NOERR;
}

#ifdef OBSOLETE
/*Allows for a .dodsrc file to be created if one does not currently exist for default authentication
 *  values*/
int
write_dodsrc(char *out_file_name)
{
    char *authent[] = { "#DODS client configuation file. See the DODS\n",
	    "#users guide for information.\n",
	    "#USE_CACHE=0\n",
	    "#Cache and object size are given in megabytes (20 ==> 20Mb).\n",
	    "#MAX_CACHE_SIZE=20\n",
	    "#MAX_CACHED_OBJ=5\n",
	    "#IGNORE_EXPIRES=0\n",
	    "#CACHE_ROOT=/Users/jimg/.dods_cache/\n",
	    "#DEFAULT_EXPIRES=86400\n",
	    "#ALWAYS_VALIDATE=0\n",
	    "# Request servers compress responses if possible?\n",
	    "# 1 (yes) or 0 (false).\n",
	    "# Should SSL certificates and hosts be validated? SSL\n",
	    "# will only work with signed certificates.\n",
	    "VALIDATE_SSL=0\n"
	    "DEFLATE=0\n",
	    "# Proxy configuration (optional parts in []s):\n",
	    "#PROXY_SERVER=http://[username:password@]host[:port]\n",
	    "#NO_PROXY_FOR=host|domain\n",
	    "# AIS_DATABASE=<file or url>\n",
	    "# The cookie jar is a file that holds cookies sent from\n",
	    "# servers such as single signon systems. Uncomment this\n",
	    "# option and provide a file name to activate this feature.\n",
	    "# If the value is a filename, it will be created in this\n",
	    "# directory; a full pathname can be used to force a specific\n",
	    "# location.\n",
	    "# COOKIE_JAR=.dods_cookies\n" };

    unsigned int i = 0;
    FILE *out_file = fopen(out_file_name, "w");
    if (out_file == NULL) {
    	oc_log(LOGERR,"cannot open output file\n");
    	return OC_EIO;
    }

    for (i = 0; i < sizeof authent / sizeof authent[0]; i++)
	fputs(authent[i], out_file);

    fclose(out_file);

    return OC_NOERR;
}
#endif
