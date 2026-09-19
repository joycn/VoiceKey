#pragma once
typedef struct { int dummy; } srmodel_list_t;
srmodel_list_t *esp_srmodel_init(const char *name);
char *esp_srmodel_filter(srmodel_list_t *models,const char *prefix,const char *filter);
