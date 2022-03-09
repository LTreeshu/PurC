/*
 * @file rdr.c
 * @author XueShuming
 * @date 2022/03/09
 * @brief The impl of the interaction between interpreter and renderer.
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

#include "purc.h"
#include "config.h"
#include "internal.h"

#include "private/errors.h"
#include "private/instance.h"
#include "private/utils.h"
#include "private/variant.h"
#include "private/pcrdr.h"

#include <string.h>

#define ID_KEY           "id"
#define TITLE_KEY        "title"
#define STYLE_KEY        "style"
#define LEVEL_KEY        "level"
#define CLASS_KEY        "class"

uintptr_t create_target_workspace(
        struct pcrdr_conn *conn_to_rdr,
        uintptr_t session_handle,
        const char *target_workspace
        )
{
    pcrdr_msg *msg = NULL;
    pcrdr_msg *response_msg = NULL;
    purc_variant_t req_data;
    uintptr_t workspace_handle = 0;

    const char *operation = PCRDR_OPERATION_CREATEWORKSPACE;
    pcrdr_msg_target target = PCRDR_MSG_TARGET_SESSION;
    uint64_t target_value = session_handle;
    msg = pcrdr_make_request_message(
            target,                             /* target */
            target_value,                       /* target_value */
            operation,                          /* operation */
            NULL,                               /* request_id */
            PCRDR_MSG_ELEMENT_TYPE_VOID,        /* element_type */
            NULL,                               /* element */
            NULL,                               /* property */
            PCRDR_MSG_DATA_TYPE_VOID,           /* data_tuype */
            NULL,                               /* data */
            0                                   /* data_len */
            );
    if (msg == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    purc_variant_t vs[2] = { NULL };
    vs[0] = purc_variant_make_string_static(TITLE_KEY, false);
    vs[1] = purc_variant_make_string_static(target_workspace, false);

    req_data = purc_variant_make_object(0, NULL, NULL);
    if (req_data == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    purc_variant_object_set(req_data, vs[0], vs[1]);
    purc_variant_unref(vs[0]);
    purc_variant_unref(vs[1]);

    msg->dataType = PCRDR_MSG_DATA_TYPE_EJSON;
    msg->data = req_data;

    if (pcrdr_send_request_and_wait_response(conn_to_rdr,
            msg, PCRDR_TIME_DEF_EXPECTED, &response_msg) < 0) {
        goto failed;
    }
    pcrdr_release_message(msg);
    msg = NULL;

    int ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK) {
        workspace_handle = response_msg->resultValue;
    }

    pcrdr_release_message(response_msg);

    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    return workspace_handle;

failed:
    if (msg)
        pcrdr_release_message(msg);

    return 0;
}


uintptr_t create_tabbed_window(
        struct pcrdr_conn *conn_to_rdr,
        uintptr_t workspace_handle,
        uintptr_t session_handle,
        const char *target_window,
        const char *target_level,
        purc_renderer_extra_info *extra_info
        )
{
    UNUSED_PARAM(target_level);

    pcrdr_msg *msg = NULL;
    pcrdr_msg *response_msg = NULL;
    purc_variant_t req_data;
    uintptr_t window_handle = 0;

    const char *operation = PCRDR_OPERATION_CREATEPLAINWINDOW;
    pcrdr_msg_target target;
    uint64_t target_value;
    if (workspace_handle) {
        target = PCRDR_MSG_TARGET_WORKSPACE;
        target_value = workspace_handle;
    }
    else {
        target = PCRDR_MSG_TARGET_SESSION;
        target_value = session_handle;
    }

    msg = pcrdr_make_request_message(
            target,                             /* target */
            target_value,                       /* target_value */
            operation,                          /* operation */
            NULL,                               /* request_id */
            PCRDR_MSG_ELEMENT_TYPE_VOID,        /* element_type */
            NULL,                               /* element */
            NULL,                               /* property */
            PCRDR_MSG_DATA_TYPE_VOID,           /* data_tuype */
            NULL,                               /* data */
            0                                   /* data_len */
            );
    if (msg == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    req_data = purc_variant_make_object(0, NULL, NULL);
    if (req_data == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    purc_variant_t vs[2] = { NULL };
    vs[0] = purc_variant_make_string_static(TITLE_KEY, false);
    vs[1] = purc_variant_make_string_static(target_window, false);
    purc_variant_object_set(req_data, vs[0], vs[1]);
    purc_variant_unref(vs[0]);
    purc_variant_unref(vs[1]);

    if (extra_info->classes) {
        vs[0] = purc_variant_make_string_static(CLASS_KEY, false);
        vs[1] = purc_variant_make_string_static(extra_info->classes, false);
        purc_variant_object_set(req_data, vs[0], vs[1]);
        purc_variant_unref(vs[0]);
        purc_variant_unref(vs[1]);
    }

    if (extra_info->styles) {
        vs[0] = purc_variant_make_string_static(STYLE_KEY, false);
        vs[1] = purc_variant_make_string_static(extra_info->styles, false);
        purc_variant_object_set(req_data, vs[0], vs[1]);
        purc_variant_unref(vs[0]);
        purc_variant_unref(vs[1]);
    }

    msg->dataType = PCRDR_MSG_DATA_TYPE_EJSON;
    msg->data = req_data;

    if (pcrdr_send_request_and_wait_response(conn_to_rdr,
            msg, PCRDR_TIME_DEF_EXPECTED, &response_msg) < 0) {
        goto failed;
    }
    pcrdr_release_message(msg);
    msg = NULL;

    int ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK) {
        window_handle = response_msg->resultValue;
    }

    pcrdr_release_message(response_msg);

    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    return window_handle;

failed:
    if (msg)
        pcrdr_release_message(msg);

    return 0;
}

uintptr_t create_tabpage(
        struct pcrdr_conn *conn_to_rdr,
        uintptr_t window_handle,
        const char *target_tabpage,
        purc_renderer_extra_info *extra_info
        )
{
    UNUSED_PARAM(extra_info);

    pcrdr_msg *msg = NULL;
    pcrdr_msg *response_msg = NULL;
    purc_variant_t req_data;
    uintptr_t page_handle = 0;

    const char *operation = PCRDR_OPERATION_CREATETABPAGE;
    pcrdr_msg_target target = PCRDR_MSG_TARGET_TABBEDWINDOW;
    uint64_t target_value = window_handle;
    msg = pcrdr_make_request_message(
            target,                             /* target */
            target_value,                       /* target_value */
            operation,                          /* operation */
            NULL,                               /* request_id */
            PCRDR_MSG_ELEMENT_TYPE_VOID,        /* element_type */
            NULL,                               /* element */
            NULL,                               /* property */
            PCRDR_MSG_DATA_TYPE_VOID,           /* data_tuype */
            NULL,                               /* data */
            0                                   /* data_len */
            );
    if (msg == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    req_data = purc_variant_make_object(0, NULL, NULL);
    if (req_data == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    purc_variant_t vs[2] = { NULL };
    vs[0] = purc_variant_make_string_static(TITLE_KEY, false);
    vs[1] = purc_variant_make_string_static(target_tabpage, false);
    purc_variant_object_set(req_data, vs[0], vs[1]);
    purc_variant_unref(vs[0]);
    purc_variant_unref(vs[1]);

    msg->dataType = PCRDR_MSG_DATA_TYPE_EJSON;
    msg->data = req_data;

    if (pcrdr_send_request_and_wait_response(conn_to_rdr,
            msg, PCRDR_TIME_DEF_EXPECTED, &response_msg) < 0) {
        goto failed;
    }
    pcrdr_release_message(msg);
    msg = NULL;

    int ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK) {
        page_handle = response_msg->resultValue;
    }

    pcrdr_release_message(response_msg);

    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    return page_handle;

failed:
    if (msg)
        pcrdr_release_message(msg);

    return 0;
}

uintptr_t create_plain_window(
        struct pcrdr_conn *conn_to_rdr,
        uintptr_t workspace_handle,
        uintptr_t session_handle,
        const char *target_window,
        const char *target_level,
        purc_renderer_extra_info *extra_info
        )
{
    UNUSED_PARAM(target_level);

    pcrdr_msg *msg = NULL;
    pcrdr_msg *response_msg = NULL;
    purc_variant_t req_data;
    uintptr_t window_handle = 0;

    const char *operation = PCRDR_OPERATION_CREATETABBEDWINDOW;
    pcrdr_msg_target target;
    uint64_t target_value;
    if (workspace_handle) {
        target = PCRDR_MSG_TARGET_WORKSPACE;
        target_value = workspace_handle;
    }
    else {
        target = PCRDR_MSG_TARGET_SESSION;
        target_value = session_handle;
    }

    msg = pcrdr_make_request_message(
            target,                             /* target */
            target_value,                       /* target_value */
            operation,                          /* operation */
            NULL,                               /* request_id */
            PCRDR_MSG_ELEMENT_TYPE_VOID,        /* element_type */
            NULL,                               /* element */
            NULL,                               /* property */
            PCRDR_MSG_DATA_TYPE_VOID,           /* data_tuype */
            NULL,                               /* data */
            0                                   /* data_len */
            );
    if (msg == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    req_data = purc_variant_make_object(0, NULL, NULL);
    if (req_data == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    purc_variant_t vs[2] = { NULL };
    vs[0] = purc_variant_make_string_static(TITLE_KEY, false);
    vs[1] = purc_variant_make_string_static(target_window, false);
    purc_variant_object_set(req_data, vs[0], vs[1]);
    purc_variant_unref(vs[0]);
    purc_variant_unref(vs[1]);

    if (extra_info->classes) {
        vs[0] = purc_variant_make_string_static(CLASS_KEY, false);
        vs[1] = purc_variant_make_string_static(extra_info->classes, false);
        purc_variant_object_set(req_data, vs[0], vs[1]);
        purc_variant_unref(vs[0]);
        purc_variant_unref(vs[1]);
    }

    if (extra_info->styles) {
        vs[0] = purc_variant_make_string_static(STYLE_KEY, false);
        vs[1] = purc_variant_make_string_static(extra_info->styles, false);
        purc_variant_object_set(req_data, vs[0], vs[1]);
        purc_variant_unref(vs[0]);
        purc_variant_unref(vs[1]);
    }

    msg->dataType = PCRDR_MSG_DATA_TYPE_EJSON;
    msg->data = req_data;

    if (pcrdr_send_request_and_wait_response(conn_to_rdr,
            msg, PCRDR_TIME_DEF_EXPECTED, &response_msg) < 0) {
        goto failed;
    }
    pcrdr_release_message(msg);
    msg = NULL;

    int ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK) {
        window_handle = response_msg->resultValue;
    }

    pcrdr_release_message(response_msg);

    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    return window_handle;

failed:
    if (msg)
        pcrdr_release_message(msg);

    return 0;
}

bool
purc_attach_vdom_to_renderer(purc_vdom_t vdom,
        const char *target_workspace,
        const char *target_window,
        const char *target_tabpage,
        const char *target_level,
        purc_renderer_extra_info *extra_info)
{
    if (!vdom) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return false;
    }

    struct pcinst *inst = pcinst_current();
    if (inst == NULL || inst->rdr_caps == NULL || target_window == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return false;
    }

    struct pcrdr_conn *conn_to_rdr = inst->conn_to_rdr;
    struct renderer_capabilities *rdr_caps = inst->rdr_caps;
    uintptr_t session_handle = rdr_caps->session_handle;

    uintptr_t workspace = 0;
    if (target_workspace && rdr_caps->workspace != 0) {
        workspace = create_target_workspace(
                conn_to_rdr,
                session_handle,
                target_workspace);
        if (!workspace) {
            purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
            return false;
        }
    }

    uintptr_t window = 0;
    uintptr_t tabpage = 0;
    if (target_tabpage) {
        window = create_tabbed_window(
                conn_to_rdr,
                workspace,
                session_handle,
                target_window, target_level, extra_info);
        if (!window) {
            purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
            return false;
        }

        tabpage = create_tabpage(conn_to_rdr, window,
                target_tabpage, extra_info);
        if (!tabpage) {
            purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
            return false;
        }
    }
    else {
        window = create_plain_window(conn_to_rdr, workspace, session_handle,
                target_window, target_level, extra_info);
        if (!window) {
            purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
            return false;
        }
    }

    pcvdom_document_set_target_workspace(vdom, workspace);
    pcvdom_document_set_target_window(vdom, window);
    pcvdom_document_set_target_tabpage(vdom, tabpage);

    return true;
}
