/*
 * @file timer.cpp
 * @author XueShuming
 * @date 2021/12/20
 * @brief The C api for timer.
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
#include "private/timer.h"
#include "private/interpreter.h"
#include "private/runloop.h"

#include <wtf/RunLoop.h>
#include <wtf/Seconds.h>

#include <stdlib.h>
#include <string.h>


class PurcTimer : public WTF::RunLoop::TimerBase {
    public:
        PurcTimer(const char* id, void* ctxt, pcintr_timer_fire_func func,
                RunLoop& runLoop)
            : TimerBase(runLoop)
            , m_id(NULL)
            , m_ctxt(ctxt)
            , m_func(func)
        {
            m_id = strdup(id);
        }

        ~PurcTimer()
        {
            if (m_id) {
                free(m_id);
            }
        }

        void setInterval(uint32_t interval) { m_interval = interval; }
        uint32_t getInterval() { return m_interval; }

        void setAttach(void* attach) { m_attach = attach; }
        void* getAttach(void) { return m_attach; }
    private:
        void fired() final { m_func(m_id, m_ctxt); }

    private:
        char* m_id;
        void* m_ctxt;
        pcintr_timer_fire_func m_func;

        uint32_t m_interval;
        void* m_attach;
};

pcintr_timer_t
pcintr_timer_create(const char* id, void* ctxt, pcintr_timer_fire_func func)
{
    PurcTimer* timer = new PurcTimer(id, ctxt, func, RunLoop::current());
    if (!timer) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    return timer;
}

void
pcintr_timer_set_interval(pcintr_timer_t timer, uint32_t interval)
{
    if (timer) {
        ((PurcTimer*)timer)->setInterval(interval);
    }
}

uint32_t
pcintr_timer_get_interval(pcintr_timer_t timer)
{
    if (timer) {
        return ((PurcTimer*)timer)->getInterval();
    }
    return 0;
}

void
pcintr_timer_start(pcintr_timer_t timer)
{
    if (timer) {
        PurcTimer* tm = (PurcTimer*)timer;
        return tm->startRepeating(
                WTF::Seconds::fromMilliseconds(tm->getInterval()));
    }
}

void
pcintr_timer_start_oneshot(pcintr_timer_t timer)
{
    if (timer) {
        PurcTimer* tm = (PurcTimer*)timer;
        return tm->startOneShot(
                WTF::Seconds::fromMilliseconds(tm->getInterval()));
    }
}

void
pcintr_timer_stop(pcintr_timer_t timer)
{
    if (timer) {
        return ((PurcTimer*)timer)->stop();
    }
}

void
pcintr_timer_destroy(pcintr_timer_t timer)
{
    if (timer) {
        PurcTimer* tm = (PurcTimer*)timer;
        delete tm;
    }
}

void
pcintr_timer_set_attach(pcintr_timer_t timer, void* attach)
{
    if (timer) {
        return ((PurcTimer*)timer)-> setAttach(attach);
    }
}

void*
pcintr_timer_get_attach(pcintr_timer_t timer)
{
    if (timer) {
        return ((PurcTimer*)timer)-> getAttach();
    }
    return NULL;
}

//  $TIMERS begin

#define TIMERS_STR_ID               "id"
#define TIMERS_STR_INTERVAL         "interval"
#define TIMERS_STR_ACTIVE           "active"
#define TIMERS_STR_ON               "on"
#define TIMERS_STR_TIMERS           "TIMERS"
#define TIMERS_STR_HANDLE           "__handle"
#define TIMERS_STR_EXPIRED          "expired"

struct pcintr_timers {
    purc_variant_t timers_var;
    struct pcvar_listener* grow_listener;
    struct pcvar_listener* shrink_listener;
};

void timer_fire_func(const char* id, void* ctxt)
{
    pcintr_stack_t stack = (pcintr_stack_t) ctxt;
    purc_variant_t type = purc_variant_make_string(TIMERS_STR_EXPIRED, false);
    purc_variant_t sub_type = purc_variant_make_string(id, false);

    pcintr_dispatch_message((pcintr_stack_t)ctxt,
            stack->vdom->timers->timers_var,
            type, sub_type, PURC_VARIANT_INVALID);

    purc_variant_unref(type);
    purc_variant_unref(sub_type);
}

bool
timer_listener_handler(purc_variant_t source, purc_atom_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv);

static bool
is_euqal(purc_variant_t var, const char* comp)
{
    if (var && comp) {
        return (strcmp(purc_variant_get_string_const(var), comp) == 0);
    }
    return false;
}

static purc_variant_t
pointer_to_variant(void* p)
{
    return p ? purc_variant_make_native(p, NULL) : PURC_VARIANT_INVALID;
}

static void*
variant_to_pointer(purc_variant_t var)
{
    if (var && purc_variant_is_type(var, PURC_VARIANT_TYPE_NATIVE)) {
        return purc_variant_native_get_entity(var);
    }
    return NULL;
}

static pcintr_timer_t
get_inner_timer(pcintr_stack_t stack, purc_variant_t timer_var)
{
    purc_variant_t tm = purc_variant_object_get_by_ckey(timer_var,
            TIMERS_STR_HANDLE);
    pcintr_timer_t timer = variant_to_pointer(tm);
    if (timer) {
        return timer;
    }

    purc_variant_t id = purc_variant_object_get_by_ckey(timer_var, TIMERS_STR_ID);
    if (!id) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    timer = pcintr_timer_create(purc_variant_get_string_const(id),
            stack, timer_fire_func);
    if (timer == NULL) {
        return NULL;
    }

    purc_variant_t native = pointer_to_variant(timer);
    purc_variant_object_set_by_static_ckey(timer_var, TIMERS_STR_HANDLE, native);
    purc_variant_unref(native);

    struct pcvar_listener* listener = purc_variant_register_post_listener(
            timer_var, pcvariant_atom_change, timer_listener_handler, stack);
    if (!listener) {
        pcintr_timer_destroy(timer);
        purc_variant_object_remove_by_static_ckey(timer_var,TIMERS_STR_HANDLE);
        return NULL;
    }
    pcintr_timer_set_attach(timer, listener);
    return timer;
}

static void
destroy_inner_timer(purc_variant_t timer_var)
{
    purc_variant_t tm = purc_variant_object_get_by_ckey(timer_var,
            TIMERS_STR_HANDLE);
    pcintr_timer_t timer = variant_to_pointer(tm);
    if (timer) {
        struct pcvar_listener* listener =
            (struct pcvar_listener*)pcintr_timer_get_attach(timer);
        purc_variant_revoke_listener(timer_var, listener);
        purc_variant_object_remove_by_static_ckey(timer_var,TIMERS_STR_HANDLE);
        pcintr_timer_destroy(timer);
    }
}

bool
timer_listener_handler(purc_variant_t source, purc_atom_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(nr_args);
    if (msg_type != pcvariant_atom_change) {
        return true;
    }
    pcintr_stack_t stack = (pcintr_stack_t) ctxt;
    pcintr_timer_t timer = get_inner_timer(stack, source);
    if (!timer) {
        return false;
    }

    // argv key-new, value-new, key-old, value-old
    if (is_euqal(argv[0], TIMERS_STR_INTERVAL)) {
        uint64_t ret = 0;
        purc_variant_cast_to_ulongint(argv[0], &ret, false);
        pcintr_timer_set_interval(timer, ret);
    }
    else if (is_euqal(argv[0], TIMERS_STR_ACTIVE)) {
        if (is_euqal(argv[1], TIMERS_STR_ON)) {
            pcintr_timer_start(timer);
        }
        else {
            pcintr_timer_stop(timer);
        }
    }
    return true;
}

bool
timers_listener_handler(purc_variant_t source, purc_atom_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(source);
    UNUSED_PARAM(msg_type);
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    pcintr_stack_t stack = (pcintr_stack_t) ctxt;
    if (msg_type == pcvariant_atom_grow) {
        purc_variant_t interval = purc_variant_object_get_by_ckey(argv[0],
                TIMERS_STR_INTERVAL);
        purc_variant_t active = purc_variant_object_get_by_ckey(argv[0],
                TIMERS_STR_ACTIVE);
        pcintr_timer_t timer = get_inner_timer(stack, argv[0]);
        if (!timer) {
            return false;
        }
        uint64_t ret = 0;
        purc_variant_cast_to_ulongint(interval, &ret, false);
        pcintr_timer_set_interval(timer, ret);
        if (is_euqal(active, TIMERS_STR_ON)) {
            pcintr_timer_start(timer);
        }
    }
    else if (msg_type == pcvariant_atom_shrink) {
        destroy_inner_timer(argv[0]);
    }
    return true;
}

struct pcintr_timers*
pcintr_timers_init(pcintr_stack_t stack)
{
    purc_variant_t ret = purc_variant_make_set_by_ckey(0, TIMERS_STR_ID, NULL);
    if (!ret) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    if (!pcintr_bind_document_variable(stack->vdom, TIMERS_STR_TIMERS, ret)) {
        purc_variant_unref(ret);
        return NULL;
    }

    struct pcintr_timers* timers = (struct pcintr_timers*) calloc(1,
            sizeof(struct pcintr_timers));
    if (!timers) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        purc_variant_unref(ret);
        return NULL;
    }

    timers->timers_var = ret;
    timers->grow_listener = purc_variant_register_post_listener(ret,
        pcvariant_atom_grow, timers_listener_handler, stack);
    if (!timers->grow_listener) {
        free(timers);
        purc_variant_unref(ret);
        return NULL;
    }

    timers->shrink_listener = purc_variant_register_post_listener(ret,
        pcvariant_atom_shrink, timers_listener_handler, stack);
    if (!timers->shrink_listener) {
        purc_variant_revoke_listener(ret, timers->grow_listener);
        free(timers);
        purc_variant_unref(ret);
        return NULL;
    }

    return timers;
}

void
pcintr_timers_destroy(struct pcintr_timers* timers)
{
    if (timers) {
        purc_variant_revoke_listener(timers->timers_var,
                timers->grow_listener);
        purc_variant_revoke_listener(timers->timers_var,
                timers->shrink_listener);

        // remove inner timer
        size_t sz;
        bool ok = purc_variant_set_size(timers->timers_var, &sz);
        if (ok) {
            for (size_t i=0; i<sz; ++i) {
                purc_variant_t v = purc_variant_set_get_by_index(
                        timers->timers_var, i);
                if (v != PURC_VARIANT_INVALID) {
                    destroy_inner_timer(v);
                }
            }
        }

        purc_variant_unref(timers->timers_var);
        free(timers);
    }
}
