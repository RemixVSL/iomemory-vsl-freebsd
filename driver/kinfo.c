//-----------------------------------------------------------------------------
// Copyright (c) 2006-2014, Fusion-io, Inc.(acquired by SanDisk Corp. 2014)
// Copyright (c) 2014-2015, SanDisk Corp. and/or all its affiliates. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of the SanDisk Corp. nor the names of its contributors
//   may be used to endorse or promote products derived from this software
//   without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
// OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include <fio/port/fio-port.h>
#include <fio/port/dbgset.h>
#include <fio/port/kinfo.h>

#include <sys/types.h>
#include <sys/sysctl.h>

typedef struct kfio_sysctl_node
{
  struct sysctl_oid     *oidp;
} kfio_sysctl_node_t;

  static int kfio_sysctl_type_proc(SYSCTL_HANDLER_ARGS)
{
    kfio_info_node_t *nodep;
    kfio_info_val_t   oldvalue;
    kfio_info_val_t   newvalue;
    fio_size_t        size = 0;

    int  rc = EINVAL;

    nodep = oidp->oid_arg1;

    oldvalue.type = kfio_info_node_get_type(nodep);
    oldvalue.size = kfio_info_node_get_size(nodep);

    newvalue.type = oldvalue.type;
    newvalue.size = oldvalue.size;

    if (oldvalue.type == KFIO_INFO_STRING)
    {
        char local_buf[32];

        oldvalue.data = local_buf;

        if (oldvalue.size > sizeof(local_buf))
        {
            size = oldvalue.size;
            oldvalue.data = kfio_vmalloc(size);
            if (oldvalue.data == NULL)
            {
                return ENOMEM;
            }
        }

        /*
         * Handle string as two disticts operations: one to read
         * old data and one to write a new one out. This saves us
         * from the need to allocate two buffers.
         *
         * Step 1: copy data from the driver
         */
        rc = kfio_info_generic_type_handler(nodep, KFIO_INFO_READ,
                                            &oldvalue, NULL);
        if (rc == 0)
        {
            /* Step 2: do data exchange. as requested by user. */
            rc = sysctl_handle_string(oidp, oldvalue.data,
                                      oldvalue.size - 1, req);
        }

        if (rc == 0)
        {
            /* Step 3: send new data into the driver, if one was given to us. */
            if (req->newptr != NULL)
            {
                rc = kfio_info_generic_type_handler(nodep, KFIO_INFO_WRITE,
                                                    NULL, &oldvalue);
            }
        }

        if (oldvalue.data != local_buf)
        {
            kfio_vfree(oldvalue.data, size);
        }
    }

    if (oldvalue.type == KFIO_INFO_UINT32 || oldvalue.type == KFIO_INFO_UINT64 ||
        oldvalue.type == KFIO_INFO_INT32)
    {
        union
        {
            uint32_t    uval;
            uint64_t    qval;
            int32_t     ival;
        } nval, oval;

        oldvalue.data = &oval;
        newvalue.data = &nval;

        if (req->newptr != NULL)
        {
            /* Fetch the new value, if one was given to us. */
            rc = SYSCTL_IN(req, newvalue.data, newvalue.size);

            if (rc == 0)
            {
                /* Perform read/write in one go. */
                rc = kfio_info_generic_type_handler(nodep, KFIO_INFO_RDWR,
                                                    &oldvalue, &newvalue);
            }
        }
        else
        {
            /* Perform just the read in one go. */
            rc = kfio_info_generic_type_handler(nodep, KFIO_INFO_READ,
                                                &oldvalue, NULL);
        }

        /* Step2: Copy out old value just fetched. */
        if (rc == 0)
        {
            rc = SYSCTL_OUT(req, oldvalue.data, oldvalue.size);
        }
    }

    return rc < 0 ? -rc : rc;
}

static int kfio_info_seqfile_handler(kfio_info_node_t *nodep,
                                     kfio_info_data_t *dbh)
{
    fio_loff_t pos;
    void      *cookie;
    int        rc;

    pos = 0;
    rc  = 0;

    cookie = kfio_info_seq_init(nodep, &pos, dbh);
    while (cookie != NULL)
    {
        rc = kfio_info_seq_show(nodep, cookie, dbh);
        if (rc != 0)
        {
            break;
        }
        cookie = kfio_info_seq_next(nodep, cookie, &pos);
    }
    kfio_info_seq_stop(nodep, cookie);
    return rc;
}

static int kfio_sysctl_text_proc(SYSCTL_HANDLER_ARGS)
{
    kfio_info_node_t *nodep;
    kfio_info_data_t *dbh;
    char             *data;
    fio_size_t        size;
    int               rc;

    nodep = oidp->oid_arg1;

    /*
     * If user just wants to calculate size required, do not bother allocating
     * data buffer.
     */
    if (req->oldptr == NULL)
    {
        data = NULL;
        size = 0;
    }
    else
    {
        size = req->oldlen - req->oldidx;

        data = kfio_vmalloc(size);
        if (data == NULL)
        {
            return ENOMEM;
        }
    }

    rc = kfio_info_alloc_data_handle(nodep, data, size, &dbh);
    if (rc == 0)
    {
        int node_type;

        node_type  = kfio_info_node_get_type(nodep);

        if (node_type == KFIO_INFO_TEXT)
        {
            rc = kfio_info_generic_text_handler(nodep, dbh);
        }
        else
        {
            rc = kfio_info_seqfile_handler(nodep, dbh);
        }

        if (rc == 0)
        {
            fio_size_t total = kfio_info_data_size_written(dbh);
            fio_size_t valid = kfio_info_data_size_valid(dbh);

            kassert (valid <= size);

            rc = SYSCTL_OUT(req, data, valid);
            kassert (rc != ENOMEM);

            /*
             * If copying out valid data portion worked, see if we need
             * to indicate an overflow.
             */
            if (rc == 0 && total > size)
            {
                req->oldidx += (total - size);

                if (req->oldptr != 0)
                    rc = ENOMEM;
            }
        }

        kfio_info_free_data_handle(dbh);
    }

    if (data != NULL)
    {
        kfio_vfree(data, size);
    }

    return rc < 0 ? -rc : rc;
}

int kfio_info_os_create_node(kfio_info_node_t *parent, kfio_info_node_t *nodep)
{
    kfio_sysctl_node_t *parent_dir;
    kfio_sysctl_node_t *node_entry;
    struct sysctl_oid_list *parent_list;

    const char *node_name;
    fio_mode_t  node_mode;
    int         node_type;
    int         ctl_mode;

    node_name  = kfio_info_node_get_name(nodep);
    node_mode  = kfio_info_node_get_mode(nodep);
    node_type  = kfio_info_node_get_type(nodep);

    /* Simulate access mode as best as we can. */
// #ifdef CTLFLAG_MPSAFE
    ctl_mode = CTLFLAG_RD | CTLFLAG_MPSAFE;
// #else
//    ctl_mode = CTLFLAG_RD;
// #endif

    if ((node_mode & S_IWUGO) != 0)
    {
        ctl_mode |= CTLFLAG_WR;
    }

    /*
     * If dir node we are creating has no parent, this is the first
     * time this code is being called. Use the opportunity to initialize
     * few globals.
     */
    if (parent == NULL)
    {
        /* Null parent_dir is legal */
        parent_dir = NULL;

        /* .. while creating top level dir only. */
        if (node_type != KFIO_INFO_DIR)
        {
            return -EIO;
        }
        parent_list = SYSCTL_STATIC_CHILDREN(_hw);
    }
    else
    {
        parent_dir = kfio_info_node_get_os_private(parent);
        if (parent_dir == NULL)
        {
            return -EIO;
        }
        parent_list = SYSCTL_CHILDREN(parent_dir->oidp);
    }

    node_entry = kfio_malloc(sizeof(*node_entry));
    if (node_entry == NULL)
    {
        return -ENOMEM;
    }
    node_entry->oidp = NULL;

    if (node_type == KFIO_INFO_DIR)
    {
        node_entry->oidp = SYSCTL_ADD_NODE(NULL, parent_list, OID_AUTO,
                                           node_name, CTLFLAG_RD,  0, "");
    }

    if (node_type == KFIO_INFO_UINT32 || node_type == KFIO_INFO_UINT64 ||
        node_type == KFIO_INFO_STRING || node_type == KFIO_INFO_INT32)
    {
        const char *fmt;
        int         type;

        switch (node_type)
        {
        case KFIO_INFO_INT32:
            type = CTLTYPE_INT;
            fmt  = "I";
            node_entry->oidp = SYSCTL_ADD_PROC(NULL, parent_list, OID_AUTO,
                                               node_name, CTLTYPE_INT | CTLFLAG_RW, nodep, 0,
                                               kfio_sysctl_type_proc, "I", "");
            break;

        case KFIO_INFO_UINT32:
            type = CTLTYPE_UINT;
            fmt  = "IU";
            node_entry->oidp = SYSCTL_ADD_PROC(NULL, parent_list, OID_AUTO,
                                   node_name, CTLTYPE_UINT | CTLFLAG_RD, nodep, 0,
                                   kfio_sysctl_type_proc, "IU", "");
            break;

#ifndef __LP64__
        /* Quad is best we can do on 32bit. */
        case KFIO_INFO_UINT64:
            type = CTLTYPE_QUAD;
            fmt  = "Q";
            node_entry->oidp = SYSCTL_ADD_PROC(NULL, parent_list, OID_AUTO,
                                   node_name, CTLTYPE_QUAD | CTLFLAG_RD, nodep, 0,
                                   kfio_sysctl_type_proc, "Q", "");
            break;
#else
        case KFIO_INFO_UINT64:
            type = CTLTYPE_ULONG;
            fmt  = "LU";
            node_entry->oidp = SYSCTL_ADD_PROC(NULL, parent_list, OID_AUTO,
                                   node_name, CTLTYPE_ULONG | CTLFLAG_RD, nodep, 0,
                                   kfio_sysctl_type_proc, "LU", "");
            break;
#endif

        case KFIO_INFO_STRING:
        default: /* should not happen, make compiler happy. */
            type = CTLTYPE_STRING;
            fmt  = "A";
            node_entry->oidp = SYSCTL_ADD_PROC(NULL, parent_list, OID_AUTO,
                                   node_name, CTLTYPE_STRING | CTLFLAG_RD, nodep, 0,
                                   kfio_sysctl_type_proc, "A", "");
            break;
        }

        /* node_entry->oidp = SYSCTL_ADD_PROC(NULL, parent_list, OID_AUTO,
                                           node_name, type | ctl_mode, nodep, 0,
                                           kfio_sysctl_type_proc, fmt, "");
         */
    }

    if (node_type == KFIO_INFO_TEXT || node_type == KFIO_INFO_SEQFILE)
    {
        node_entry->oidp = SYSCTL_ADD_PROC(NULL, parent_list, OID_AUTO,
                                           node_name, CTLTYPE_STRING | CTLFLAG_RD, nodep, 0,
                                           kfio_sysctl_text_proc, "A", "");
    }

    if (node_entry->oidp == NULL)
    {
        kfio_free(node_entry, sizeof(*node_entry));
        return -EIO;
    }

    kfio_info_node_set_os_private(nodep, node_entry);
    return 0;
}

void kfio_info_os_remove_node(kfio_info_node_t *parent, kfio_info_node_t *nodep)
{
//    kfio_sysctl_node_t *parent_dir;
    kfio_sysctl_node_t *node_entry;

    const char *node_name;
    int         node_type;

    node_name  = kfio_info_node_get_name(nodep);
    node_type  = kfio_info_node_get_type(nodep);

    node_entry = kfio_info_node_get_os_private(nodep);

    /* Deleting entry that is not there is always OK. */
    if (node_entry == NULL)
    {
        return;
    }

    kfio_info_node_set_os_private(nodep, NULL);

    sysctl_remove_oid(node_entry->oidp, 1 /*del*/, 1 /*recurse*/);
    kfio_free(node_entry, sizeof(*node_entry));
}
