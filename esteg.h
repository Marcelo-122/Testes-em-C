#ifndef STEG_H
#define STEG_H

#include <stddef.h>

int steg_hide(const char *image_path, const unsigned char *data, 
              size_t data_size, const char *output_path);


int steg_extract(const char *image_path, unsigned char **data, 
                 size_t *data_size);


int steg_hide_file(const char *image_path, const char *file_path, 
                   const char *output_path);


int steg_extract_file(const char *image_path, const char *output_path);


long steg_get_capacity(const char *image_path);

#endif 