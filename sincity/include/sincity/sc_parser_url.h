#ifndef SINCITY_PARSER_URL_H
#define SINCITY_PARSER_URL_H

#include "sc_config.h"
#include "sincity/sc_common.h"
#include "sincity/sc_url.h"

SCObjWrapper<SCUrl*> sc_url_parse(const char *urlstring, tsk_size_t length);


#endif /* SINCITY_PARSER_URL_H */
