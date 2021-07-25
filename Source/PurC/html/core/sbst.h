/**
 * @file sbst.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for sbst.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PCHTML_SBST_H
#define PCHTML_SBST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

#include "config.h"
#include "html/core/base.h"


typedef struct {
    unsigned char key;

    void       *value;
    size_t     value_len;

    size_t     left;
    size_t     right;
    size_t     next;
}
pchtml_sbst_entry_static_t;


/*
 * Inline functions
 */
static inline const pchtml_sbst_entry_static_t *
pchtml_sbst_entry_static_find(const pchtml_sbst_entry_static_t *strt,
                              const pchtml_sbst_entry_static_t *root,
                              const unsigned char key)
{
    while (root != strt) {
        if (root->key == key) {
            return root;
        }
        else if (key > root->key) {
            root = &strt[root->right];
        }
        else {
            root = &strt[root->left];
        }
    }

    return NULL;
}

#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_SBST_H */
